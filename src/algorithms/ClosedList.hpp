#pragma once

#include "SearchNode.hpp"
#include "Environment.h"

#include <vector>
#include <algorithm>

class ClosedList
{

public:

    ClosedList() {}

    virtual void add(State c) = 0;
    virtual bool contains(State c) const = 0;
    virtual void clear() = 0;
    virtual size_t size() const = 0;
    virtual std::vector<State> getStates() const = 0;

    bool empty() const
    {
        return size() == 0;
    }
};

class ClosedListVector : public ClosedList
{
    const Environment* m_env = nullptr;
    std::vector<State> m_closed;

public:

    ClosedListVector(const Environment* env = nullptr) 
        : m_env(env)
    {}

    void add(State c)
    {
        m_closed.push_back(c);
    }

    bool contains(State c) const
    {
        auto it = std::find(m_closed.begin(), m_closed.end(), c);
        return it != m_closed.end();
    }

    std::vector<State> getStates() const
    {
        return m_closed;
    }

    size_t size() const
    {
        return m_closed.size();
    }

    void clear()
    {
        m_closed.clear();
    }
};

class ClosedListMap : public ClosedList
{
    const Environment* m_env = nullptr;
    Grid<bool> m_closed;
    size_t m_numClosed = 0;
    
public:

    ClosedListMap(const Environment* env = nullptr)
        : m_env(env)
        , m_closed(env->width(), env->height(), false)
    {}

    void add(State c)
    {
        m_closed.set(c, true);
    }

    bool contains(State c) const
    {
        return m_closed.get(c);
    }

    std::vector<State> getStates() const
    {
        std::vector<State> states;
        for (uint32_t x = 0; x < m_env->width(); x++)
        {
            for (uint32_t y = 0; y < m_env->height(); y++)
            {
                if (m_closed.get(x, y))
                {
                    states.push_back(State(x, y));
                }
            }
        }
        return states;
    }

    size_t size() const
    {
        return 0;
    }

    void clear()
    {
        m_closed.clear(false);
    }
};
