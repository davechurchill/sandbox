#pragma once

#include "Environment.h"

class GridConnectivity
{
    const Environment* m_env = nullptr;

    Grid<int32_t> m_sectors;

    int32_t getSector(State c)
    {
        return m_sectors.get(c);
    }

    void computeConnectivity()
    {
        int32_t sectorNumber = 0;
        for (uint32_t x = 0; x < m_env->width(); x++)
        {
            for (uint32_t y = 0; y < m_env->height(); y++)
            {
                State c(x, y);

                // if we haven't yet assigned a sector number to this cell, do a BFS
                if (m_sectors.get(c) == 0)
                {
                    floodFill(c, ++sectorNumber);
                }
            }
        }
    }

    void floodFill(State s, int32_t sector) 
    {
        std::vector<State> open = { s };
        open.reserve(m_env->size());
        
        for (uint32_t f = 0; f < open.size(); f++) 
        {
            State c = open[f];

            // if we are out of bounds, stop
            if (m_env->isOOB(c)) { continue; }

            // if we have already computed a sector, stop
            if (m_sectors.get(c) != 0) { continue; }

            // if the tile is not the same terrain, stop
            if (m_env->get(c) != m_env->get(s)) { continue; }

            m_sectors.set(c, sector);

            open.emplace_back(State(c.x + 1, c.y + 0));
            open.emplace_back(State(c.x - 1, c.y + 0));
            open.emplace_back(State(c.x + 0, c.y + 1));
            open.emplace_back(State(c.x + 0, c.y - 1));
        }
    }

public:

    GridConnectivity() {}

    GridConnectivity(const Environment* env)
        : m_env(env) 
        , m_sectors(env->width(), env->height(), 0)
    {  
        computeConnectivity();
    }

    bool isConnected(State c1, State c2) const
    {
        return !m_env->isOOB(c1) && !m_env->isOOB(c2) && (m_sectors.get(c1) == m_sectors.get(c2));
    }
};
