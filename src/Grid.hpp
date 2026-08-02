#pragma once

#include <vector>


template <class T>
class Grid
{
    size_t m_width = 0;
    size_t m_height = 0;

    std::vector<T> m_grid;

public:
    Grid() = default;

    Grid(size_t width, size_t height, T val)
        : m_width(width)
        , m_height(height)
        , m_grid(width* height, val)
    {

    }

    inline T& get(size_t x, size_t y)
    {
        return m_grid[y * m_width + x];
    }

    inline const T& get(size_t x, size_t y) const
    {
        return m_grid[y * m_width + x];
    }

    inline void add(size_t x, size_t y, T val)
    {
        m_grid[y * m_width + x] += val;
    }

    inline void set(size_t x, size_t y, T val)
    {
        m_grid[y * m_width + x] = val;
    }

    inline size_t width() const
    {
        return m_width;
    }

    inline size_t height() const
    {
        return m_height;
    }

    inline T * data()
    {
        return m_grid.data();
    }
};
