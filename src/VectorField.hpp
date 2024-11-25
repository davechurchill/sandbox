#pragma once

#include <opencv2/core.hpp>
#include <queue>

#include "Grid.hpp"
#include "SFML/System/Vector2.hpp"

namespace VectorField
{
    Grid<sf::Vector2<float>> compute(const cv::Mat& grid, int spacing, float terrainWeight)
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

        Grid<float> m_cost = Grid<float>(gridWidth + 1, gridHeight, 0); // average terrain height
        Grid<float> m_bestCost = Grid<float>(gridWidth + 1, gridHeight, std::numeric_limits<float>::max());
        Grid<sf::Vector2<float>> m_directions = Grid<sf::Vector2<float>>(gridWidth, gridHeight, { 0, 0 }); // director vector

        for (int x = 0; x < gridWidth; ++x)
        {
            for (int y = 0; y < gridHeight; ++y)
            {
                int count = 0;
                float sum = 0;

                for (int sx = 0; sx < spacing && x * spacing + sx < pixelWidth; ++sx)
                {
                    for (int sy = 0; sy < spacing && y * spacing + sy < pixelHeight; ++sy)
                    {
                        count++;
                        sum += grid.at<float>(y * spacing + sy, x * spacing + sx);
                    }
                }

                m_cost.set(x, y, sum / count);
            }
        }

        std::queue<sf::Vector2<int>> openList{};

        for (int y = 0; y < gridHeight; ++y)
        {
            openList.push({ gridWidth, y });
            m_bestCost.set(gridWidth, y, 0);
        }

        while (!openList.empty())
        {
            sf::Vector2i cell = openList.front();
            openList.pop();

            auto currBestCost = m_bestCost.get(cell.x, cell.y);;

            auto update = [&](int x, int y)
            {
                float& neighbourBestCost = m_bestCost.get(x, y);
                float newCost = currBestCost + (1 - terrainWeight) + m_cost.get(x, y) * terrainWeight;
                
                if (newCost < neighbourBestCost)
                {
                    openList.push({ x, y });
                    neighbourBestCost = newCost;
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
                auto bestCost = m_bestCost.get(x, y);

                float left = x > 0 ? m_bestCost.get(x - 1, y) : bestCost;
                float right = x < gridWidth - 1 ? m_bestCost.get(x + 1, y) : bestCost;
                float up = y > 0 ? m_bestCost.get(x, y - 1) : bestCost;
                float down = y < gridHeight - 1 ? m_bestCost.get(x, y + 1) : bestCost;

                sf::Vector2<float> dir{ left - right, up - down };

                float magnitude = std::sqrt(dir.x * dir.x + dir.y * dir.y);

                m_directions.get(x, y) = { dir.x / magnitude, dir.y / magnitude };
            }
        }

        return m_directions;
    }
}
