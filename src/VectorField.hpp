#pragma once

constexpr double PI = 3.1415926535897932384626433832;

#include <complex>
#include <fstream>
#include <opencv2/core.hpp>
#include <queue>

#include "Grid.hpp"
#include "SFML/System/Vector2.hpp"

namespace VectorField
{
    ////////////////////////
    // BFS
    ////////////////////////

    Grid<sf::Vector2<double>> computeBFS(const cv::Mat& grid, int spacing, float terrainWeight)
    {
        int pixelWidth = grid.cols;
        int pixelHeight = grid.rows;

        int gridWidth = pixelWidth / spacing;
        if (pixelWidth % spacing > 0) {
            gridWidth += 1;
        }

        int gridHeight = pixelHeight / spacing;
        if (pixelHeight % spacing > 0) {
            gridHeight += 1;
        }

        Grid<double> m_grid = Grid<double>(gridWidth, gridHeight, 0); // average terrain height
        Grid<double> m_distance = Grid<double>(gridWidth + 1, gridHeight, -1); // distance to nearest goal cell * (1 + terrain height * weight) 
        Grid<sf::Vector2<double>> m_directions = Grid<sf::Vector2<double>>(gridWidth, gridHeight, { 0, 0 }); // director vector

        for (int x = 0; x < gridWidth; ++x)
        {
            for (int y = 0; y < gridHeight; ++y)
            {
                int count = 0;
                double sum = 0;

                for (int sx = 0; sx < spacing && x * spacing + sx < pixelWidth; ++sx)
                {
                    for (int sy = 0; sy < spacing && y * spacing + sy < pixelHeight; ++sy)
                    {
                        count++;
                        sum += grid.at<float>(y * spacing + sy, x * spacing + sx);
                    }
                }

                m_grid.set(x, y, sum / count);
            }
        }

        std::queue<sf::Vector2<int>> openList{};

        for (int y = 0; y < gridHeight; ++y)
        {
            openList.push({ gridWidth, y });
            m_distance.set(gridWidth, y, 0);
        }

        while (!openList.empty())
        {
            sf::Vector2i cell = openList.front();
            openList.pop();

            auto value = m_distance.get(cell.x, cell.y);

            auto update = [&](int x, int y)
            {
                auto& dist = m_distance.get(x, y);
                if (dist == -1)
                {
                    openList.push({ x, y });
                    dist = value + 1;
                }
            };

            if (cell.x > 0)
            {
                update(cell.x - 1, cell.y);
            }
            if (cell.x < gridWidth)
            {
                update(cell.x + 1, cell.y);
            }
            if (cell.y > 0)
            {
                update(cell.x, cell.y - 1);
            }
            if (cell.y < gridHeight - 1)
            {
                update(cell.x, cell.y + 1);
            }
        }

        for (int x = 0; x < gridWidth; ++x)
        {
            for (int y = 0; y < gridHeight; ++y)
            {
                m_distance.get(x, y) *= 1 + m_grid.get(x, y) * terrainWeight;
            }
        }

        for (int x = 0; x < gridWidth; ++x)
        {
            for (int y = 0; y < gridHeight; ++y)
            {
                auto thisDist = m_distance.get(x, y);

                double left = x > 0 ? m_distance.get(x - 1, y) : thisDist;
                double right = x < gridWidth - 1 ? m_distance.get(x + 1, y) : thisDist;
                double up = y > 0 ? m_distance.get(x, y - 1) : thisDist;
                double down = y < gridHeight - 1 ? m_distance.get(x, y + 1) : thisDist;

                sf::Vector2<double> dir{ left - right, up - down };

                double magnitude = std::sqrt(dir.x * dir.x + dir.y * dir.y);

                m_directions.get(x, y) = { dir.x / magnitude, dir.y / magnitude };
            }
        }

        return m_directions;
    }

    ////////////////////////
    // Charney
    ////////////////////////

    const int SUMMATION_POINTS = 80;

    const double PHI = PI / 4;
    const double S = 2.5;
    const double U = 0.29;
    const double G = 9.8;
    const double OMEGA = 7.27 * std::pow(10, -5);

    const double S_SQUARED = S * S;
    const double BETA = 4 * PI * std::pow(std::cos(PHI), 2);
    const double LAMBDA_SQUARED = 2.5 * std::pow(std::sin(2 * PHI), 2);
    const double M_SQUARED = BETA / U - S_SQUARED;
    const double F = 2 * OMEGA * sin(PHI);
    const double G_OVER_F = G / F;

    using Matrix = std::vector<std::vector<double>>;

    Matrix zeroMatrix(int width, int height) {
        return Matrix(height, std::vector<double>(width, 0.0));
    }

    struct ComputeContext {
        double friction;
        double windVelocity;
        double reductionFactor;

        int width;
        int height;
        double dx;
        double dy;

        std::vector<double> x;
        std::vector<double> ySin;
        std::vector<double> h;

        Matrix u;
        Matrix v;

        ComputeContext(const cv::Mat& grid, double friction, double windVelocity, double reductionFactor = 0.4) :
            friction(friction),
            windVelocity(windVelocity),
            reductionFactor(reductionFactor)
        {
            width = grid.cols;
            height = grid.rows;

            dx = 2 * PI / width;
            dy = PI / height;

            x = std::vector<double>(width);
            ySin = std::vector<double>(height);
            h = std::vector<double>(width);

            u = zeroMatrix(width, height);
            v = zeroMatrix(width, height);

            for (int x = 0; x < width; ++x)
            {
                this->x[x] = x * dx;

                double sum = 0.0;

                for (int y = 0; y < height; ++y)
                {
                    sum += grid.at<float>(y, (int)x);
                }

                h[x] = sum / height;
            }

            for (int y = 0; y < height; ++y)
            {
                ySin[y] = sin(y * dy);
            }
        }
    };

    double greensTerm(ComputeContext& context, int n, int nSquared, double x)
    {
        std::complex<double> numerator = std::exp(std::complex<double>(0, n * x));
        std::complex<double> denominator = nSquared - S_SQUARED - std::complex<double>(0, context.friction * (n + M_SQUARED) / n);
        return (numerator / denominator).real();
    }

    double greens(ComputeContext& context, double x)
    {
        double sum = 0;

        for (int n = 1; n <= SUMMATION_POINTS / 2; ++n)
        {
            int nSquared = n * n;
            sum += greensTerm(context, n, nSquared, x) + greensTerm(context, -n, nSquared, x);
        }

        return sum / (2.0 * PI);
    }

    double integrate(ComputeContext& context, const std::vector<double>& integrand)
    {
        double integral = 0.0;

        for (int x = 1; x < context.width; ++x)
        {
            // This should be multiplied by 2, but...
            integral += PI / context.width * (integrand[x] + integrand[x - 1]);
        }

        // We would need to divide it by 2 here, so they cancel out!
        return integral;
    }

    std::vector<double> z(ComputeContext& context)
    {
        std::vector<double> result(context.width);

        for (int x = 0; x < context.width; ++x)
        {
            std::vector<double> integrand(context.width);

            for (int x2 = 0; x2 < context.width; ++x2)
            {
                integrand[x2] = context.h[x2] * greens(context, context.x[x] - context.x[x2]);
            }

            result[x] = context.reductionFactor * LAMBDA_SQUARED * integrate(context, integrand);
        }

        return result;
    }

    void computeWindTrajectories(ComputeContext& context, double reduction_factor = 0.4)
    {
        // Compute z Matrix

        auto zValues = z(context);

        Matrix zMat = zeroMatrix(context.width, context.height);

        for (int x = 0; x < context.width; ++x)
        {
            for (int y = 0; y < context.height; ++y)
            {
                zMat[y][x] = zValues[x] * context.ySin[y];
            }
        }

        // Compute raw trajectories

#define compute_v(a, b) (zMat[y][a] - zMat[y][b]) / context.dx * G_OVER_F

        for (int y = 1; y < context.height - 1; ++y)
        {
            for (int x = 0; x < context.width; ++x)
            {
                context.u[y][x] = (zMat[y + 1][x] - zMat[y - 1][x]) / context.dy * -G_OVER_F;
            }

            for (int x = 1; x < context.width - 1; ++x)
            {
                context.v[y][x] = compute_v(x + 1, x - 1);
            }

            // Handle edge cases
            context.v[y][0] = compute_v(0, context.width - 1);
            context.v[y][context.width - 1] = compute_v(context.width - 1, 0);
        }

        // Normalize u and v and apply constant velocity

        double normalFactor = 0.0;

        for (int x = 0; x < context.width; ++x)
        {
            for (int y = 0; y < context.height; ++y)
            {
                const double absU = std::abs(context.u[y][x]);
                const double absV = std::abs(context.v[y][x]);
                normalFactor = std::max(normalFactor, std::max(absU, absV));
            }
        }

        for (int x = 0; x < context.width; ++x)
        {
            for (int y = 0; y < context.height; ++y)
            {
                context.u[y][x] = context.u[y][x] / normalFactor + context.windVelocity;
                context.v[y][x] = context.v[y][x] / normalFactor;
            }
        }
    }

    Grid<sf::Vector2<double>> computePhysics(const cv::Mat& grid, bool compute)
    {
        static Grid<sf::Vector2<double>> directions;
        static bool directionsInitialized = false;

        if (compute || !directionsInitialized) {
            ComputeContext context{ grid, 0.5, 0.4 };

            computeWindTrajectories(context);

            directions = Grid<sf::Vector2<double>>(context.width, context.height, { 0, 0 });
            directionsInitialized = true;

            for (size_t x = 0; x < context.width; ++x)
            {
                for (size_t y = 0; y < context.height; ++y)
                {
                    directions.set(x, y, { context.u[y][x], context.v[y][x] });
                }
            }
        }

        return directions;
    }
}
