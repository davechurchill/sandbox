#pragma once

#include "Action.hpp"   
#include "Environment.h"
#include "GridConnectivity.hpp"

#include <iostream>

class GridLegalActions
{
    const Environment*        m_env          = nullptr;
    const GridConnectivity*   m_connectivity = nullptr;
    std::vector<Action>       m_actions;
    Grid<std::vector<Action>> m_gridLegal;
    
    void computeLegalActions()
    {
        for (uint32_t x = 0; x < m_env->width(); x++)
        {
            for (uint32_t y = 0; y < m_env->height(); y++)
            {
                State c(x, y);
                m_gridLegal.get(c).reserve(8);

                for (auto action : m_actions)
                {
                    if (isLegalAction(c, action))
                    {
                        m_gridLegal.get(c).emplace_back(action);
                    }
                }

                //std::cout << c.x << " " << c.y << " " << m_gridLegal.get(c).size() << "\n";
            }
        }
    }



public:

    GridLegalActions() {}

    GridLegalActions(const Environment& env, const GridConnectivity& connectivity, const std::vector<Action>& actions)
        : m_env(&env)
        , m_connectivity(&connectivity)
        , m_actions(actions)
        , m_gridLegal(env.width(), env.height(), std::vector<Action>())
    {  
        computeLegalActions();
    }

    bool isLegalAction(State state, const Action& action)
    {
        return m_connectivity->isConnected(state, state + action.dir)
            && m_connectivity->isConnected(state, state + State(action.dir.x, 0))
            && m_connectivity->isConnected(state, state + State(0, action.dir.y));
    }

    const std::vector<Action>& getLegalActions(State state) const
    {
        return m_gridLegal.get(state);
    }
};
