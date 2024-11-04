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

Grid<std::vector<Direction>> VectorField::compute(const cv::Mat& grid, size_t spacing, size_t gx, size_t gy)
{
    m_spacing = spacing;
    m_width = grid.size().width / m_spacing;
    m_height = grid.size().height / m_spacing;
    m_gx = gx;
    m_gy = gy;

    m_grid = Grid<double>(m_width, m_height, 0);
    m_distance = Grid<int>(m_width, m_height, -1);
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
    openList.push({ m_gx, m_gy });
    m_distance.set(m_gx, m_gy, 0);

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
        if (cell.x < m_distance.width() - 1)
        {
            update(cell.x + 1, cell.y);
        }
        if (cell.y > 0)
        {
            update(cell.x, cell.y - 1);
        }
        if (cell.y < m_distance.height() - 1)
        {
            update(cell.x, cell.y + 1);
        }
    }

    for (int x = 0; x < m_distance.width(); ++x)
    {
        for (int y = 0; y < m_distance.height(); ++y)
        {
            if (x == gx && y == gy)
            {
                continue;
            }

            double min = std::numeric_limits<double>::max();

            auto get_v = [&](int sx, int sy)
            {
                return m_distance.get(sx, sy) * (1 - m_grid.get(x, y) + m_grid.get(sx, sy));
            };

            auto update_min = [&](int sx, int sy)
            {
                min = std::min((double) min, get_v(sx, sy));
            };

            if (x > 0)
            {
                update_min(x - 1, y);
            }
            if (x < m_distance.width() - 1)
            {
                update_min(x + 1, y);
            }
            if (y > 0)
            {
                update_min(x, y - 1);
            }
            if (y < m_distance.height() - 1)
            {
                update_min(x, y + 1);
            }

            auto add_direction = [&](int dx, int dy)
            {
                if (get_v(x + dx, y + dy) == min)
                {
                    m_directions.get(x, y).push_back({ dx, dy });
                }
            };

            if (x > 0)
            {
                add_direction(-1, 0);
            }
            if (x < m_distance.width() - 1)
            {
                add_direction(1, 0);
            }
            if (y > 0)
            {
                add_direction(0, -1);
            }
            if (y < m_distance.height() - 1)
            {
                add_direction(0, 1);
            }
        }
    }

    return m_directions;
}

bool VectorField::isGoalCell(size_t x, size_t y) const
{
    return x == m_gx && y == m_gy;
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
