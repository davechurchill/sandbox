#include "VectorField.h"

#include <cassert>
#include <math.h>
#include <algorithm>
#include <iostream>
#include <queue>
#include <functional>

VectorField::VectorField()
{
}

Grid<std::vector<Direction>> VectorField::compute(const cv::Mat& grid, size_t spacing)
{
    m_spacing = spacing;
    m_width = grid.size().width / m_spacing;
    m_height = grid.size().height / m_spacing;

    m_grid = Grid<double>(m_width, m_height, 0);
    m_distance = Grid<double>(m_width, m_height, -1);
    m_directions = Grid<std::vector<Direction>>(m_width, m_height, {});

    for (int x = 0; x < m_width; ++x)
    {
        for (int y = 0; y < m_height; ++y)
        {
            double sum = 0;

            for (int sx = 0; sx < m_spacing; ++sx)
            {
                for (int sy = 0; sy < m_spacing; ++sy)
                {
                    sum += grid.at<float>(y * m_spacing + sy, x * m_spacing + sx);
                }
            }

            m_grid.set(x, y, sum / (m_spacing * m_spacing));
        }
    }

    std::queue<Cell> openList{};

    for (size_t y = 0; y < m_height; ++y)
    {
        openList.push({ m_width - 1, y });
        m_distance.set(m_width - 1, y, 0);
    }

    while (!openList.empty())
    {
        auto cell = openList.front();
        openList.pop();

        auto value = m_distance.get(cell.x, cell.y);

        auto update = [&](size_t x, size_t y)
            {
                auto& cell = m_distance.get(x, y);
                if (cell == -1)
                {
                    openList.push(Cell{ x, y });
                    cell = value + 1;
                }
            };

        if (cell.x > 0)
        {
            update(cell.x - 1, cell.y);
        }
        if (cell.x < m_width - 1)
        {
            update(cell.x + 1, cell.y);
        }
        if (cell.y > 0)
        {
            update(cell.x, cell.y - 1);
        }
        if (cell.y < m_height - 1)
        {
            update(cell.x, cell.y + 1);
        }
    }

    double weight = 0.2;
    for (int x = 0; x < m_width; ++x)
    {
        for (int y = 0; y < m_height; ++y)
        {
            m_distance.get(x, y) *= 1 + m_grid.get(x, y) * weight;
        }
    }

    for (int x = 0; x < m_width; ++x)
    {
        for (int y = 0; y < m_height; ++y)
        {
            constexpr double max = std::numeric_limits<double>::max();

            double left = x > 0 ? m_distance.get(x - 1, y) : max;
            double right = x < m_width - 1 ? m_distance.get(x + 1, y) : max;
            double up = y > 0 ? m_distance.get(x, y - 1) : max;
            double down = y < m_height - 1 ? m_distance.get(x, y + 1) : max;

            Direction dir{ left - right, up - down };

            double magnitude = std::sqrt(dir.x * dir.x + dir.y * dir.y);

            m_directions.get(x, y).push_back({ dir.x / magnitude, dir.y / magnitude });
        }
    }

    return m_directions;
}

bool VectorField::isValidCell(size_t x, size_t y) const
{
    return x >= 0 && y >= 0 && x < width() && y < height();
}

int VectorField::getGrid(size_t x, size_t y) const
{
    return m_grid.get(x, y);
}

int VectorField::getDistance(size_t x, size_t y) const
{
    return m_distance.get(x, y);
}

size_t VectorField::getNumDirections(size_t x, size_t y) const
{
    return m_directions.get(x, y).size();
}

Direction VectorField::getDirection(size_t x, size_t y, size_t index) const
{
    return m_directions.get(x, y)[index];
}
