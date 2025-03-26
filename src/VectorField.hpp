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

    cv::Mat zeroMatrix(int width, int height) {
        return cv::Mat::zeros(height, width, CV_64F);
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
        std::vector<double> integrand;

        cv::Mat zMat;
        cv::Mat u;
        cv::Mat v;

        ComputeContext(const cv::Mat& grid, double friction, double windVelocity, double reductionFactor = 0.4) :
            friction(friction),
            windVelocity(windVelocity),
            reductionFactor(reductionFactor)
        {
            update(grid);
        }

        void update(const cv::Mat& grid) {
            bool dimensionsChanged = false;

            if (grid.cols != width)
            {
                dimensionsChanged = true;

                width = grid.cols;
                dx = 2 * PI / width;

                x = std::vector<double>(width);
                h = std::vector<double>(width);
                integrand = std::vector<double>(width);

                for (int xIndex = 0; xIndex < width; ++xIndex)
                {
                    this->x[xIndex] = xIndex * dx;
                }
            }

            if (grid.rows != height)
            {
                dimensionsChanged = true;

                height = grid.rows;
                dy = PI / height;

                ySin = std::vector<double>(height);

                for (int yIndex = 0; yIndex < height; ++yIndex)
                {
                    ySin[yIndex] = sin(yIndex * dy);
                }
            }

            if (dimensionsChanged)
            {
                zMat = cv::Mat::zeros(height, width, CV_64F);
                u = cv::Mat::zeros(height, width, CV_64F);
                v = cv::Mat::zeros(height, width, CV_64F);

                for (int j = 0; j < width; ++j)
                {
                    double sum = 0.0;

                    for (int i = 0; i < height; ++i)
                    {
                        sum += grid.at<float>(i, j);
                    }

                    h[j] = sum / height;
                }
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

    double integrate(ComputeContext& context)
    {
        double result = 0.0;

        for (int x = 1; x < context.width; ++x)
        {
            // This should be multiplied by 2, but...
            result += PI / context.width * (context.integrand[x] + context.integrand[x - 1]);
        }

        // We would need to divide it by 2 here, so they cancel out!
        return result;
    }

    double z(ComputeContext& context, int xIndex)
    {
        for (int alpha = 0; alpha < context.width; ++alpha)
        {
            context.integrand[alpha] = context.h[alpha] * greens(context, context.x[xIndex] - context.x[alpha]);
        }

        return context.reductionFactor * LAMBDA_SQUARED * integrate(context);
    }

    void computeWindTrajectories(ComputeContext& context, double reduction_factor = 0.4)
    {
        // Compute z matrix

        for (int x = 0; x < context.width; ++x)
        {
            const auto zValue = z(context, x);

            for (int y = 0; y < context.height; ++y)
            {
                context.zMat.at<double>(y, x) = zValue * context.ySin[y];
            }
        }

        // Compute raw trajectories

        for (int y = 1; y < context.height - 1; ++y)
        {
            for (int x = 0; x < context.width; ++x)
            {
                context.u.at<double>(y, x) = (context.zMat.at<double>(y + 1, x) - context.zMat.at<double>(y - 1, x)) / context.dy * -G_OVER_F;
            }

            context.v.at<double>(y, 0) = (context.zMat.at<double>(y, 0) - context.zMat.at<double>(y, context.width - 1)) / context.dx * G_OVER_F;
            for (int x = 1; x < context.width - 1; ++x)
            {
                context.v.at<double>(y, x) = (context.zMat.at<double>(y, x + 1) - context.zMat.at<double>(y, x - 1)) / context.dx * G_OVER_F;
            }
            context.v.at<double>(y, context.width - 1) = (context.zMat.at<double>(y, context.width - 1) - context.zMat.at<double>(y, 0)) / context.dx * G_OVER_F;
        }

        // Normalize u and v and apply constant velocity

        double minU, maxU, minV, maxV;
        cv::minMaxLoc(context.u, &minU, &maxU);
        cv::minMaxLoc(context.u, &minV, &maxV);

        const double absMaxU = std::max(std::abs(minU), maxU);
        const double absMaxV = std::max(std::abs(minV), maxV);
        const double normalFactor = std::max(absMaxU, absMaxV);

        cv::parallel_for_(cv::Range(0, context.width * context.height), [&](const cv::Range& range) {
            for (int r = range.start; r < range.end; ++r) {
                int x = r % context.width;
                int y = r / context.width;

                auto& u = context.u.at<double>(y, x);
                auto& v = context.v.at<double>(y, x);

                u = u / normalFactor + context.windVelocity;
                v = v / normalFactor;
            }
        });
    }

    Grid<sf::Vector2<double>> computePhysics(const cv::Mat& grid, bool compute)
    {
        static Grid<sf::Vector2<double>> directions;
        static bool directionsInitialized = false;
        static ComputeContext context{ grid, 0.5, 0.4 };

        if (compute || !directionsInitialized) {
            context.update(grid);

            computeWindTrajectories(context);

            directions = Grid<sf::Vector2<double>>(context.width, context.height, { 0, 0 });
            directionsInitialized = true;

            for (size_t x = 0; x < context.width; ++x)
            {
                for (size_t y = 0; y < context.height; ++y)
                {
                    directions.set(x, y, { context.u.at<double>(y, x), context.v.at<double>(y, x) });
                }
            }
        }

        return directions;
    }
}
