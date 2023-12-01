#pragma once

#include <vector>
#include <string>
#include "../Grid.hpp"

class Environment
{
    std::string m_path;
    Grid<char>  m_grid;
    size_t    m_width = 0;
    size_t    m_height = 0;
    size_t    m_count[256] = {};

public:

    Environment();
    Environment(const std::string& path);

    const std::string& getPath() const;

    size_t getCount(char val) const;
    size_t width() const;
    size_t height() const;
    size_t size() const;
    bool isOOB(State c) const;
    
    char get(State c) const;
};