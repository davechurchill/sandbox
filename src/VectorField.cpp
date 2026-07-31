#pragma once

#include <algorithm>
#include <complex>
#include <fstream>
#include <limits>
#include <opencv2/core.hpp>
#include <queue>

#include "Grid.hpp"
#include "SFML/System/Vector2.hpp"
#include "VectorField.h"

////////////////////////
// BFS
////////////////////////

cv::Mat VectorField::computeBFS(
    const cv::Mat& grid,
    int spacing,
    float heightPenalty)
{
    spacing = std::max(spacing, 1);
    heightPenalty = std::max(heightPenalty, 0.0f);

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
    Grid<double> m_distance = Grid<double>(
        gridWidth + 1,
        gridHeight,
        std::numeric_limits<double>::infinity()); // height-adjusted distance to the goal
    cv::Mat m_directions = cv::Mat::zeros(gridHeight, gridWidth, CV_64FC2); // director vector

    for (int x = 0; x < gridWidth; ++x)
    {
        for (int y = 0; y < gridHeight; ++y)
        {
            int sampleCount = 0;
            double heightSum = 0.0;

            for (int sx = 0; sx < spacing && x * spacing + sx < pixelWidth; ++sx)
            {
                for (int sy = 0; sy < spacing && y * spacing + sy < pixelHeight; ++sy)
                {
                    const float height = grid.at<float>(y * spacing + sy, x * spacing + sx);
                    if (std::isfinite(height))
                    {
                        heightSum += std::clamp((double)height, 0.0, 1.0);
                        sampleCount++;
                    }
                }
            }

            m_grid.set(x, y, sampleCount > 0 ? heightSum / sampleCount : 1.0);
        }
    }

    struct QueueEntry
    {
        double distance;
        sf::Vector2i cell;
    };
    const auto fartherFromGoal = [](const QueueEntry & left, const QueueEntry & right)
    {
        return left.distance > right.distance;
    };
    std::priority_queue<
        QueueEntry,
        std::vector<QueueEntry>,
        decltype(fartherFromGoal)> openList(fartherFromGoal);

    for (int y = 0; y < gridHeight; ++y)
    {
        openList.push({ 0.0, { gridWidth, y } });
        m_distance.set(gridWidth, y, 0.0);
    }

    while (!openList.empty())
    {
        const QueueEntry entry = openList.top();
        openList.pop();
        const sf::Vector2i cell = entry.cell;
        const double value = m_distance.get(cell.x, cell.y);
        if (entry.distance > value)
        {
            continue;
        }

        auto update = [&](int x, int y)
        {
            const double terrainCost = x < gridWidth
                ? m_grid.get(x, y) * heightPenalty
                : 0.0;
            const double candidateDistance = value + 1.0 + terrainCost;
            double& dist = m_distance.get(x, y);
            if (candidateDistance < dist)
            {
                dist = candidateDistance;
                openList.push({ candidateDistance, { x, y } });
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
            double thisDist = m_distance.get(x, y);
            if (!std::isfinite(thisDist))
            {
                m_directions.at<cv::Vec2d>(y, x) = { 0.0, 0.0 };
                continue;
            }

            sf::Vector2<double> dir{ 0.0, 0.0 };
            const auto addDescent = [&](int sampleX, int sampleY, double xDirection, double yDirection)
            {
                const double sampleDistance = m_distance.get(sampleX, sampleY);
                if (sampleDistance < thisDist)
                {
                    const double strength = thisDist - sampleDistance;
                    dir.x += xDirection * strength;
                    dir.y += yDirection * strength;
                }
            };
            if (x > 0) addDescent(x - 1, y, -1.0, 0.0);
            addDescent(x + 1, y, 1.0, 0.0);
            if (y > 0) addDescent(x, y - 1, 0.0, -1.0);
            if (y < gridHeight - 1) addDescent(x, y + 1, 0.0, 1.0);

            double magnitude = std::sqrt(dir.x * dir.x + dir.y * dir.y);

            m_directions.at<cv::Vec2d>(y, x) = magnitude > 0.000001
                ? cv::Vec2d(dir.x / magnitude, dir.y / magnitude)
                : cv::Vec2d(0.0, 0.0);
        }
    }

    return m_directions;
}

////////////////////////
// Charney
////////////////////////

VectorField::ComputeContext::ComputeContext(const cv::Mat& grid, double friction, double windVelocity, double reductionFactor) :
    friction(friction),
    windVelocity(windVelocity),
    reductionFactor(reductionFactor)
{
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
            x[xIndex] = xIndex * dx;
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
        uv = cv::Mat::zeros(height, width, CV_64FC2);

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

inline double VectorField::greensTerm(double friction, int n, double nSquared_Minus_sSquared, double x)
{
    std::complex<double> numerator = std::exp(std::complex<double>(0, n * x));
    std::complex<double> denominator = nSquared_Minus_sSquared - std::complex<double>(0, friction * (n + M_SQUARED) / n);
    return (numerator / denominator).real();
}

inline double VectorField::greens(double friction, double x)
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

void VectorField::ComputeContext::computeWindTrajectories()
{
    // Compute z matrix

    cv::parallel_for_(cv::Range(0, width), [&](const cv::Range& range) {
        for (int xIndex = range.start; xIndex < range.end; ++xIndex) {
            const double xValue = x[xIndex];

            // Integrate

            const auto integrandTerm = [&, xValue](int alpha) {
                return h[alpha] * greens(friction, xValue - x[alpha]);
            };

            double integralSum = integrandTerm(0) + integrandTerm(width - 1);

            for (int alpha = 1; alpha < width - 1; ++alpha)
            {
                integralSum += 2 * integrandTerm(alpha);
            }

            const double integrationResult = piOverWidth * integralSum;

            // Compute Z

            const double zValue = reductionFactor * LAMBDA_SQUARED * integrationResult;

            // Expand to 2D

            for (int yIndex = 0; yIndex < height; ++yIndex)
            {
                zMat.at<double>(yIndex, xIndex) = zValue * ySin[yIndex];
            }
        }
    });

    // Compute raw trajectories

    // Iterating over [1, height - 1) and [0, width)
    cv::parallel_for_(cv::Range(0, (height - 2) * width), [&](const cv::Range& range) {
        for (int r = range.start; r < range.end; ++r)
        {
            int x = r % width;
            int y = r / width + 1;  // +1 to account for the y = 1 starting point

            int xPlus = (x + 1) % width;
            int xMinus = (x - 1 + width) % width;

            cv::Vec2d& vec = uv.at<cv::Vec2d>(y, x);

            vec[0] = (zMat.at<double>(y + 1, x) - zMat.at<double>(y - 1, x)) / dy * -G_OVER_F;
            vec[1] = (zMat.at<double>(y, xPlus) - zMat.at<double>(y, xMinus)) / dx * G_OVER_F;

        }
    });

    double normalFactor = 0.0;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const cv::Vec2d& vec = uv.at<cv::Vec2d>(y, x);
            normalFactor = std::max(normalFactor, std::max(std::abs(vec[0]), std::abs(vec[1])));
        }
    }

    // Normalize u and v and apply constant velocity
    if (normalFactor <= 1e-12)
    {
        uv.setTo(cv::Scalar(windVelocity, 0.0));
    }
    else
    {
        uv.forEach<cv::Vec2d>([&, normalFactor](cv::Vec2d& vec, const int* position) {
            vec /= normalFactor;
            vec[0] += windVelocity;
        });
    }
}

cv::Mat VectorField::computePhysics(const cv::Mat& grid, bool compute)
{
    static bool initialized = false;
    static cv::Mat directions{};

    if (!initialized)
    {
        initialized = true;
        compute = true;
    }

    if (compute) {
        ComputeContext context{ grid, 0.5, 0.4 };
        context.computeWindTrajectories();
        directions = context.uv.clone();
    }

    return directions;
}
