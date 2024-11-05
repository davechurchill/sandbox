#pragma once

#include "Grid.hpp"

#include <opencv2/core.hpp>
#include <vector>

struct Direction
{
    int x = 0;
    int y = 0;
};

struct Cell { size_t x = 0; size_t y = 0; };

class VectorField
{
    size_t m_spacing    = 16;   // spacing of the grid (pixels)
    size_t m_width      = 0;    // number of columns in the grid
    size_t m_height     = 0;    // number of rows in the grid

    Grid<double> m_grid;           // blocked/unblocked grid
    Grid<int> m_distance;       // distance map grid
    Grid<std::vector<Direction>> m_directions; // vector field direction grid

public:

    VectorField();

    // called by the GUI to compute the grid, distance map, and vector field
    Grid<std::vector<Direction>> compute(const cv::Mat& data, size_t spacing);

    // returns the value of the blocked grid (0 = unblocked, 1 = blocked)
    int getGrid(size_t x, size_t y) const;

    // returns the value of the disance map at cell (x,y)
    int getDistance(size_t x, size_t y) const;

    // returns the directions (vectors) of the vector field at cell (x,y)
    size_t getNumDirections(size_t x, size_t y) const;
    Direction getDirection(size_t x, size_t y, size_t index) const;

    // various helper functions
    bool isValidCell(size_t x, size_t y) const;
    inline size_t width() const { return m_width; }
    inline size_t height() const { return m_height; }
    inline size_t spacing() const { return m_spacing; }
};
