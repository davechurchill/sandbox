#pragma once

#include "SearchNode.hpp"

#include <vector>
#include <algorithm>

class OpenList
{
    

public:

    OpenList() {}
    virtual ~OpenList() {}

    virtual Node* pop() = 0;
    virtual void push(Cell s, Node* p, Action a, int32_t ng, int32_t nh) = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;
    virtual std::vector<Cell> getStates() const = 0;

    bool empty() const
    {
        return size() == 0;
    }
};

class OpenListVector : public OpenList
{
    std::vector<Node*> m_open;
    std::vector<Node*> m_allNodes;

public:

    OpenListVector() 
    {
        m_open.reserve(1000);
    }

    ~OpenListVector()
    {
        clear();
    }

    Node* pop()
    {
        auto it = std::min_element(m_open.begin(), m_open.end(), MinFMinH());
        Node* min = *it;
        m_open.erase(it);
        return min;
    }
    
    void push(Cell s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        Node* n = new Node(s, p, a, ng, nh);
        m_open.push_back(n);
        m_allNodes.push_back(n);
    }

    std::vector<Cell> getStates() const
    {
        std::vector<Cell> open;
        for (const Node* n : m_open)
        {
            open.push_back(n->state);
        }
        return open;
    }

    size_t size() const
    {
        return m_open.size();
    }

    void clear()
    {
        for (Node* n : m_allNodes) { delete n; }
        m_allNodes.clear();
        m_open.clear();
    }
};

class OpenListVectorSPtr : public OpenList
{
    std::vector<std::shared_ptr<Node>> m_open;

public:

    OpenListVectorSPtr() {}

    Node* pop()
    {
        auto it = std::min_element(m_open.begin(), m_open.end(), MinFMinHSPtr());
        Node* min = it->get();
        m_open.erase(it);
        return min;
    }

    void push(Cell s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        m_open.push_back(std::make_shared<Node>(s, p, a, ng, nh));
    }

    std::vector<Cell> getStates() const
    {
        std::vector<Cell> open;
        for (auto& n : m_open)
        {
            open.push_back(n->state);
        }
        return open;
    }

    size_t size() const
    {
        return m_open.size();
    }

    void clear()
    {
        m_open.clear();
    }
};

class OpenListVectorUPtr : public OpenList
{
    std::vector<std::unique_ptr<Node>> m_open;

public:

    OpenListVectorUPtr() {}

    Node* pop()
    {
        auto it = std::min_element(m_open.begin(), m_open.end(), MinFMinHUPtr());
        Node* min = it->get();
        m_open.erase(it);
        return min;
    }

    void push(Cell s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        m_open.push_back(std::make_unique<Node>(s, p, a, ng, nh));
    }

    std::vector<Cell> getStates() const
    {
        std::vector<Cell> open;
        for (auto& n : m_open)
        {
            open.push_back(n->state);
        }
        return open;
    }

    size_t size() const
    {
        return m_open.size();
    }

    void clear()
    {
        m_open.clear();
    }
};

class OpenListVectorSorted : public OpenList
{
    std::vector<Node*> m_open;
    std::vector<Node*> m_allNodes;

public:

    OpenListVectorSorted() 
    {
        m_open.reserve(1000);
    }

    ~OpenListVectorSorted()
    {
        clear();
    }

    Node* pop()
    {
        Node* n = m_open.back();
        m_open.pop_back();
        return n;
    }

    void push(Cell s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        size_t index = 0;
        Node* next = new Node(s, p, a, ng, nh);
        for (auto n : m_open)
        {
            if (!MinFMinH()(next, n)) { break; }
            index++;
        }

        //m_open.push_back(new Node(s, p, a, ng, nh));
        m_open.insert(m_open.begin() + index, next);
        m_allNodes.push_back(next);
    }

    std::vector<Cell> getStates() const
    {
        std::vector<Cell> open;
        for (const Node* n : m_open)
        {
            open.push_back(n->state);
        }
        return open;
    }

    size_t size() const
    {
        return m_open.size();
    }

    void clear()
    {
        for (Node* n : m_allNodes) { delete n; }
        m_allNodes.clear();
        m_open.clear();
    }
};

#include <queue>
class OpenListPQ : public OpenList
{
    std::priority_queue<Node*, std::vector<Node*>, MinFMinHPQ> m_open;
    std::vector<Node*> m_allNodes;

public:

    OpenListPQ() {}

    Node* pop()
    {
        Node* n = m_open.top();
        m_open.pop();
        return n;
    }

    ~OpenListPQ()
    {
        clear();
    }

    void push(Cell s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        Node* n = new Node(s, p, a, ng, nh);
        m_allNodes.push_back(n);
        m_open.push(n);
    }

    std::vector<Cell> getStates() const
    {
        return {};
    }

    size_t size() const
    {
        return m_open.size();
    }

    void clear()
    {
        for (auto n : m_allNodes) { delete n; }
        m_allNodes.clear();
        m_open = {};
    }
};

#include <queue>
class OpenListMap : public OpenList
{
    Grid<Node> m_nodes;
    size_t m_count = 0;

public:

    OpenListMap(const Environment& env) 
    {
        for (size_t x = 0; x < env.width(); x++)
        {
            for (size_t y = 0; y < env.height(); y++)
            {
                m_nodes.get(x, y).state.x = (int32_t)x;
                m_nodes.get(x, y).state.x = (int32_t)y;
            }
        }
    }

    Node* pop()
    {
        m_count--;


    }

    void push(Cell s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        auto& node = m_nodes.get(s);
        if (node.action.cost == 0)
        {
            node = Node(s, p, a, ng, nh);
            //node.action = a;
            //node.parent = p;
            //node.g = ng;
            //node.h = nh;
            //node.f = node.g + node.h;
        }
    }

    std::vector<Cell> getStates() const
    {
        return {};
    }

    size_t size() const
    {
        return 1;
    }

    void clear()
    {

    }
};