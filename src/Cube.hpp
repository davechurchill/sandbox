#pragma once

#include <vector>
#include <algorithm>


template <class T>
class Cube
{
    size_t m_sizeX = 0;
    size_t m_sizeY = 0;
    size_t m_sizeZ = 0;

    std::vector<T> m_cube;

public:

    Cube() {}

    Cube(size_t sizeX, size_t sizeY, size_t sizeZ, T val)
        : m_sizeX(sizeX)
        , m_sizeY(sizeY)
        , m_sizeZ(sizeZ)
        , m_cube(sizeX * sizeY * sizeZ, val)
    {

    }

    inline size_t xyz_i(size_t x, size_t y, size_t z) const { return x * m_sizeY * m_sizeZ + y * m_sizeZ + z; }

    void normalize()
    {
        T max = maxVal();
        if (max != 0)
        {
            for (T & val : m_cube)
            {
                val /= max;
            }
        }
    }

    T maxVal()
    {
        return *std::max_element(std::begin(m_cube), std::end(m_cube));
    }

    T minVal()
    {
        return *std::min_element(std::begin(m_cube), std::end(m_cube));
    }

    inline void refill(size_t sizeX, size_t sizeY, size_t sizeZ, T val)
    {
        if (sizeX == m_sizeX && sizeY == m_sizeY && sizeZ == m_sizeZ)
        {
            std::fill(m_cube.begin(), m_cube.end(), val);
        }
        else
        {
            m_sizeX = sizeX;
            m_sizeY = sizeY;
            m_sizeZ = sizeZ;
            m_cube = std::vector<T>(sizeX * sizeY * sizeZ, val);
        }
    }

    void fill(int x1, int y1, int z1, int x2, int y2, int z2, const T & val)
    {
        int xstep = (x2 - x1 >= 0) ? 1 : -1;
        int ystep = (y2 - y1 >= 0) ? 1 : -1;
        int zstep = (z2 - z1 >= 0) ? 1 : -1;

        for (int x = x1; x <= x2; x += xstep)
        {
            for (int y = y1; y <= y2; y += ystep)
            {
                for (int z = z1; z <= z2; z += zstep)
                {
                    set(x,y,z,val);
                }
            }
        }

    }

    void clear(T val)
    {
        std::fill(m_cube.begin(), m_cube.end(), val);
    }

    inline T& get(size_t index)
    {
        return m_cube[index];
    }

    inline const T& get(size_t index) const
    {
        return m_cube[index];
    }

    inline void set(size_t index, T val)
    {
        m_cube[index] = val;
    }

    inline T& get(size_t x, size_t y, size_t z)
    {
        return m_cube[xyz_i(x,y,z)];
    }

    inline const T& get(size_t x, size_t y, size_t z) const
    {
        return m_cube[xyz_i(x, y, z)];
    }

    inline void set(size_t x, size_t y, size_t z, T val)
    {
        m_cube[xyz_i(x, y, z)] = val;
    }

    inline size_t sizeX() const
    {
        return m_sizeX;
    }

    inline size_t sizeY() const
    {
        return m_sizeY;
    }

    inline size_t sizeZ() const
    {
        return m_sizeZ;
    }

    inline T * data()
    {
        return m_cube.data();
    }
};