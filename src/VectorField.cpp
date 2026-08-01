#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

#include "Grid.hpp"
#include "SFML/System/Vector2.hpp"
#include "VectorField.h"

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
