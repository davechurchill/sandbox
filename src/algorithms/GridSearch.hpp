#pragma once

#include "Environment.h"
#include "GridConnectivity.hpp"
#include "GridLegalActions.hpp"
#include "SearchNode.hpp"
#include "OpenList.hpp"
#include "ClosedList.hpp"
#include "Timer.hpp"

#include <memory>
#include <string>


class GridSearch
{
    const Environment*  m_env = nullptr;
    GridConnectivity    m_connect;
    GridLegalActions    m_legalActions;

    // start and goal positions
    State                m_start;
    State                m_goal;

    bool                 m_inProgress = false;

    size_t              m_expanded = 0;
    int32_t              m_cost = 0;
    std::vector<State>    m_path;

    std::shared_ptr<OpenList> m_open;
    std::shared_ptr<ClosedList> m_closed;

    void reconstructPath(Node* node)
    {
        Node* n = node;
        while (n)
        {
            m_path.push_back(n->state);
            n = n->parent;
        }
    }

public:

    GridSearch() {}

    GridSearch(const Environment& env)
        : m_env(&env) 
        , m_connect(&env)
    { 
        m_legalActions = GridLegalActions(*m_env, m_connect, Actions8());

        m_open      = std::make_shared<OpenListPQ2>(*m_env);
        m_closed    = std::make_shared<ClosedListMap>(m_env);
    }

    void setOpenList(std::shared_ptr<OpenList> open)
    {
        m_open = open;
    }

    void setClosedList(std::shared_ptr<ClosedList> closed)
    {
        m_closed = closed;
    }

    void startSearch(State start, State goal)
    {
        m_inProgress = true;
        m_start = start;
        m_goal = goal;

        m_cost = 0;
        m_expanded = 0;
        m_path.clear();
        m_closed->clear();
        m_open->clear();

        m_open->push(m_start, nullptr, Action(), 0, estimateCost(m_start, m_goal));
        //m_open->push(std::make_shared<NodeBase>(m_start, nullptr, Cell(0, 0), 0, estimateCost(m_start, m_goal)));
    }

    void search()
    {
        while (m_inProgress)
        {
            searchIteration();
        }
    }

    void searchIteration()
    {
        if (!m_inProgress) { return; }

        // do a quick check to see if the start and goal positions are connected
        if (!m_connect.isConnected(m_start, m_goal))
        {
            m_inProgress = false;
            m_cost = -1;
            return;
        }

        // the search is over (fail) when the open list is empty
        if (m_open->empty())
        {
            m_inProgress = false;
            m_cost = -1;
            return;
        }

        // get the minimum element from the open list
        Node* node = m_open->pop();

        if (m_closed->contains(node->state))
        {
            return;
        }

        m_expanded++;
        // if we found a solution, set the path
        if (node->state == m_goal)
        {
            reconstructPath(node);
            m_cost = node->g;
            m_inProgress = false;
            return;
        }

        // otherwise, add the children
        const auto& legalActions = m_legalActions.getLegalActions(node->state);

        for (auto action : legalActions)
        {
            State childState = node->state + action.dir;
            if (m_closed->contains(childState)) { continue; }

            m_open->push(childState, node, action, node->g + action.cost, estimateCost(childState, m_goal));
        }

        m_closed->add(node->state);
    }

    int32_t estimateCost(State from, State to) 
    {
        //return 0;

        State d = from.absdiff(to);

        if (d.x > d.y)  { return d.y * 141 + (d.x - d.y) * 100; }
        else            { return d.x * 141 + (d.y - d.x) * 100; }
    }

    bool isRunning() const
    {
        return m_inProgress;
    }

    size_t getExpanded() const
    {
        return m_expanded;
    }

    std::vector<State> getOpen() const
    {
        return m_open->getStates();
    }

    std::vector<State> getClosed() const
    {
        return m_closed->getStates();
    }

    const std::vector<State>& getPath() const
    {
        return m_path;
    }

    int32_t getCost() const
    {
        return m_cost;
    }
};
