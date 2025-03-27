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

    inline cv::Mat zeroMatrix(int width, int height) {
        return cv::Mat::zeros(height, width, CV_64F);
    }

    inline double greensTerm(double friction, int n, double nSquared_Minus_sSquared, double x)
    {
        std::complex<double> numerator = std::exp(std::complex<double>(0, n * x));
        std::complex<double> denominator = nSquared_Minus_sSquared - std::complex<double>(0, friction * (n + M_SQUARED) / n);
        return (numerator / denominator).real();
    }

    inline double greens(double friction, double x)
    {
        double sum = 0;

        for (int n = 1; n <= SUMMATION_POINTS / 2; ++n)
        {
            const double nSquared_Minus_sSquared = n * n - S_SQUARED;
            const double positiveTerm = greensTerm(friction, n, nSquared_Minus_sSquared, x);
            const double negativeTerm = greensTerm(friction, -n, nSquared_Minus_sSquared, x);
            sum += positiveTerm + negativeTerm;
        }

        return sum / (2.0 * PI);
    }

    struct ComputeContext {
        double friction;
        double windVelocity;
        double reductionFactor;

        int width = -1;
        int height = -1;
        double dx = 1;
        double dy = 1;

        std::vector<double> x;
        std::vector<double> ySin;
        std::vector<double> h;

        cv::Mat zMat;
        cv::Mat u;
        cv::Mat v;

        double piOverWidth = 1;

        ComputeContext(double friction, double windVelocity, double reductionFactor = 0.4) :
            friction(friction),
            windVelocity(windVelocity),
            reductionFactor(reductionFactor)
        {
        }

        // This function does very little work unless dimensions have changed,
        // so not a target for optimization
        bool update(const cv::Mat& grid) {
            bool dimensionsChanged = false;

            if (grid.cols != width)
            {
                dimensionsChanged = true;

                width = grid.cols;
                dx = 2 * PI / width;

                x = std::vector<double>(width);
                h = std::vector<double>(width);

                for (int xIndex = 0; xIndex < width; ++xIndex)
                {
                    this->x[xIndex] = xIndex * dx;
                }

                piOverWidth = PI / width;
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

            return dimensionsChanged;
        }

        inline double integrate(const cv::Mat& integrand) const
        {
            const double integrandSum = cv::sum(integrand)[0];
            const double adjustedSum = integrandSum - integrand.at<double>(0, 0) - integrand.at<double>(0, width - 1);
            return piOverWidth * adjustedSum;
        }

        inline double z(int xIndex) const
        {
            cv::Mat integrand(1, width, CV_64F);

            integrand.forEach<double>([&](double& value, const int* position) {
                const int alpha = position[1];
                value = h[alpha] * greens(friction, x[xIndex] - x[alpha]);
                });

            return reductionFactor * LAMBDA_SQUARED * integrate(integrand);
        }

        // Compute wind trajectories
        void compute()
        {
            // Compute z matrix

            cv::parallel_for_(cv::Range(0, width), [&](const cv::Range& range) {
                for (int x = range.start; x < range.end; ++x) {
                    const auto zValue = z(x);

                    for (int y = 0; y < height; ++y)
                    {
                        zMat.at<double>(y, x) = zValue * ySin[y];
                    }
                }
                });

            // Compute raw trajectories

            cv::parallel_for_(cv::Range(0, (height - 2) * width), [&](const cv::Range& range) {
                for (int i = range.start; i < range.end; ++i)
                {
                    int y = i / width + 1;  // +1 to account for the y = 1 starting point
                    int x = i % width;

                    u.at<double>(y, x) = (zMat.at<double>(y + 1, x) - zMat.at<double>(y - 1, x)) / dy * -G_OVER_F;

                    int xPlus = (x + 1) % width;
                    int xMinus = (x - 1 + width) % width;

                    v.at<double>(y, x) = (zMat.at<double>(y, xPlus) - zMat.at<double>(y, xMinus)) / dx * G_OVER_F;
                }
                });

            // Normalize u and v and apply constant velocity

            double minU, maxU, minV, maxV;
            cv::minMaxLoc(u, &minU, &maxU);
            cv::minMaxLoc(v, &minV, &maxV);

            const double absMaxU = std::max(std::abs(minU), maxU);
            const double absMaxV = std::max(std::abs(minV), maxV);
            const double normalFactor = std::max(absMaxU, absMaxV);

            cv::parallel_for_(cv::Range(0, width * height), [&](const cv::Range& range) {
                for (int r = range.start; r < range.end; ++r) {
                    int x = r % width;
                    int y = r / width;

                    auto& uValue = u.at<double>(y, x);
                    auto& vValue = v.at<double>(y, x);

                    uValue = uValue / normalFactor + windVelocity;
                    vValue = vValue / normalFactor;
                }
            });
        }
    };

    Grid<sf::Vector2<double>> computePhysics(const cv::Mat& grid, bool compute)
    {
        static bool initialized = false;
        static Grid<sf::Vector2<double>> directions;
        static ComputeContext context{ 0.5, 0.4 };

        if (!initialized)
        {
            initialized = true;
            compute = true;
        }

        if (compute) {
            const bool dimensionsChanged = context.update(grid);
            context.compute();

            // Will always be true the first time the function is called,
            // so 'directions' will always be initialized
            if (dimensionsChanged)
            {
                directions = Grid<sf::Vector2<double>>(context.width, context.height, { 0, 0 });
            }

            cv::parallel_for_(cv::Range(0, context.width * context.height), [&](const cv::Range& range) {
                for (int r = range.start; r < range.end; ++r) {
                    int x = r % context.width;
                    int y = r / context.width;

                    directions.set(x, y, { context.u.at<double>(y, x), context.v.at<double>(y, x) });
                }
            });
        }

        return directions;
    }
}
