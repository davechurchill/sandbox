#include "Environment.h"
#include <iostream>
#include <fstream>

// based on the movingai map format
// https://movingai.com/benchmarks/formats.html
//
// type octile
// height y
// width x
// map
// 
// . - passable terrain
// G - passable terrain
// @ - out of bounds
// O - out of bounds
// T - trees(unpassable)
// S - swamp(passable from regular terrain)
// W - water(traversable, but not passable from terrain)


Environment::Environment()
{

}

Environment::Environment(const std::string& path)
    : m_path(path)
{
    std::ifstream fin(path);
    std::string token;

    fin >> token >> token >> token >> m_height >> token >> m_width >> token;
    
    m_grid = Grid<char>(m_width, m_height, 0);
            
    // read in the values for each tile of the map
    char value;
    for (size_t y = 0; y < m_height; y++)
    {
        for (size_t x = 0; x < m_width; x++)
        {
            fin >> value;
            m_count[value]++;
            m_grid.set(x, y, value);
        }
    }
}

bool Environment::isOOB(State c) const
{
    if (c.x < 0) { return true; }
    if (c.y < 0) { return true; }
    if (c.x >= (int)width()) { return true; }
    if (c.y >= (int)height()) { return true; }
    return false;
}

const std::string& Environment::getPath() const
{
    return m_path;
}

size_t Environment::getCount(char val) const
{
    return m_count[val];
}

size_t Environment::width() const
{
    return m_width;
}

size_t Environment::height() const
{
    return m_height;
}

char Environment::get(State c) const
{
    return m_grid.get(c.x, c.y);
}

size_t Environment::size() const
{
    return m_width * m_height;
}