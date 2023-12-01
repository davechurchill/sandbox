#pragma once

#include "SearchNode.hpp"

#include <vector>
#include <set>
#include <algorithm>


class OpenList
{
    

public:

    OpenList() {}
    virtual ~OpenList() {}

    virtual Node* pop() = 0;
    virtual void push(State s, Node* p, Action a, int32_t ng, int32_t nh) = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;
    virtual std::vector<State> getStates() const = 0;

    bool empty() const
    {
        return size() == 0;
    }
};

#include <list>
#include <deque>
#include <vector>

template <class T>
class OpenListLinear : public OpenList
{
    T m_open;
    std::vector<Node*> m_allNodes;

public:

    OpenListLinear()
    {
        m_allNodes.reserve(10000);
    }

    ~OpenListLinear()
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

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        Node* n = new Node(s, p, a, ng, nh);
        m_open.push_back(n);
        m_allNodes.push_back(n);
    }

    std::vector<State> getStates() const
    {
        std::vector<State> open;
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

#include <list>
#include <type_traits>
class OpenListList : public OpenList
{
    std::list<Node*> m_open;
    std::vector<Node*> m_allNodes;

public:

    OpenListList()
    {
        m_allNodes.reserve(10000);
    }

    ~OpenListList()
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
    
    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        Node* n = new Node(s, p, a, ng, nh);
        m_open.push_back(n);
        m_allNodes.push_back(n);
    }

    std::vector<State> getStates() const
    {
        std::vector<State> open;
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

class OpenListVector2 : public OpenList
{
    std::vector<Node*> m_open;
    std::vector<Node> m_allNodes;

public:

    OpenListVector2()
    {
        m_open.reserve(1000000);
        m_allNodes.reserve(1000000);
    }

    ~OpenListVector2()
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

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        m_allNodes.push_back(Node(s, p, a, ng, nh));
        m_open.push_back(&m_allNodes.back());
    }

    std::vector<State> getStates() const
    {
        std::vector<State> open;
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
        m_allNodes.clear();
        m_open.clear();
    }
};

class OpenListVector : public OpenList
{
    std::vector<Node*> m_open;
    std::vector<Node*> m_allNodes;

public:

    OpenListVector()
    {
        m_open.reserve(10000);
        m_allNodes.reserve(10000);
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

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        Node* n = new Node(s, p, a, ng, nh);
        m_open.push_back(n);
        m_allNodes.push_back(n);
    }

    std::vector<State> getStates() const
    {
        std::vector<State> open;
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

#include <deque>
class OpenListDeque : public OpenList
{
    std::deque<Node*> m_open;
    std::vector<Node*> m_allNodes;

public:

    OpenListDeque()
    {
        m_allNodes.reserve(10000);
    }

    ~OpenListDeque()
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

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        Node* n = new Node(s, p, a, ng, nh);
        m_open.push_back(n);
        m_allNodes.push_back(n);
    }

    std::vector<State> getStates() const
    {
        std::vector<State> open;
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
    std::vector<Node*> m_open;
    std::vector<std::shared_ptr<Node>> m_allNodes;

public:

    OpenListVectorSPtr() 
    {
        m_open.reserve(1000);
        m_allNodes.reserve(1000);
    }

    Node* pop()
    {
        auto it = std::min_element(m_open.begin(), m_open.end(), MinFMinH());
        Node* min = *it;
        m_open.erase(it);
        return min;
    }

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        m_allNodes.push_back(std::make_shared<Node>(s, p, a, ng, nh));
        m_open.push_back(m_allNodes.back().get());
    }

    std::vector<State> getStates() const
    {
        std::vector<State> open;
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
        m_allNodes.clear();
    }
};

class OpenListVectorUPtr : public OpenList
{
    std::vector<Node*> m_open;
    std::vector<std::unique_ptr<Node>> m_allNodes;

public:

    OpenListVectorUPtr() 
    {
        m_open.reserve(1000);
        m_allNodes.reserve(1000);
    }

    Node* pop()
    {
        auto it = std::min_element(m_open.begin(), m_open.end(), MinFMinH());
        Node* min = *it;
        m_open.erase(it);
        return min;
    }

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        m_allNodes.push_back(std::make_unique<Node>(s, p, a, ng, nh));
        m_open.push_back(m_allNodes.back().get());
    }

    std::vector<State> getStates() const
    {
        std::vector<State> open;
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
        m_allNodes.clear();
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

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        Node* next = new Node(s, p, a, ng, nh);
        auto it = m_open.begin();
        for (; it != m_open.end(); it++)
        {
            if (!MinFMinH()(next, *it)) { break; }
        }

        //m_open.push_back(new Node(s, p, a, ng, nh));
        m_open.insert(it, next);
        m_allNodes.push_back(next);
    }

    std::vector<State> getStates() const
    {
        std::vector<State> open;
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

#include <deque>
class OpenListDequeSorted : public OpenList
{
    std::deque<Node*> m_open;
    std::vector<Node*> m_allNodes;

public:

    OpenListDequeSorted()
    {
        m_allNodes.reserve(1000);
    }

    ~OpenListDequeSorted()
    {
        clear();
    }

    Node* pop()
    {
        Node* n = m_open.back();
        m_open.pop_back();
        return n;
    }

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        Node* next = new Node(s, p, a, ng, nh);
        auto it = m_open.begin();
        for (; it != m_open.end(); it++)
        {
            if (!MinFMinH()(next, *it)) { break; }
        }

        //m_open.push_back(new Node(s, p, a, ng, nh));
        m_open.insert(it, next);
        m_allNodes.push_back(next);
    }

    std::vector<State> getStates() const
    {
        std::vector<State> open;
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

#include <list>
class OpenListListSorted : public OpenList
{
    std::list<Node*> m_open;
    std::vector<Node*> m_allNodes;

public:

    OpenListListSorted()
    {
        m_allNodes.reserve(1000);
    }

    ~OpenListListSorted()
    {
        clear();
    }

    Node* pop()
    {
        Node* n = m_open.back();
        m_open.pop_back();
        return n;
    }

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        Node* next = new Node(s, p, a, ng, nh);
        auto it = m_open.begin();
        for (; it != m_open.end(); it++)
        {
            if (!MinFMinH()(next, *it)) { break; }
        }

        //m_open.push_back(new Node(s, p, a, ng, nh));
        m_open.insert(it, next);
        m_allNodes.push_back(next);
    }

    std::vector<State> getStates() const
    {
        std::vector<State> open;
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
template <class T>
class OpenListPQ : public OpenList
{
    std::priority_queue<Node*, T, MinFMinHPQ> m_open;
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

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        Node* n = new Node(s, p, a, ng, nh);
        m_allNodes.push_back(n);
        m_open.push(n);
    }

    std::vector<State> getStates() const
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

class OpenListMapVector : public OpenList
{
    Grid<Node> m_nodes;
    std::vector<Node*> m_open;

public:

    OpenListMapVector(const Environment& env) 
        : m_nodes(env.width(), env.height(), Node())
    {
        m_open.reserve(10000);
    }

    Node* pop()
    {
        auto it = std::min_element(m_open.begin(), m_open.end(), MinFMinH());
        Node* min = *it;
        m_open.erase(it);
        return min;
    }

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        auto& node = m_nodes.get(s);

        if (node.parent && node.g <= ng) { return; }
        //printf("\nPush%6d%6d\n", s.x, s.y);

        m_open.push_back(&node);
        node = Node(s, p, a, ng, nh);
    }

    std::vector<State> getStates() const
    {
        return {};
    }

    size_t size() const
    {
        return m_open.size();
    }

    void clear()
    {
        m_nodes.clear(Node());
        m_open.clear();
    }
};

#include <deque>
class OpenListMapDeque : public OpenList
{
    Grid<Node> m_nodes;
    std::deque<Node*> m_open;

public:

    OpenListMapDeque(const Environment& env)
        : m_nodes(env.width(), env.height(), Node())
    {
        
    }

    Node* pop()
    {
        auto it = std::min_element(m_open.begin(), m_open.end(), MinFMinH());
        Node* min = *it;
        m_open.erase(it);
        min->inOpen = false;
        return min;
    }

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        auto& node = m_nodes.get(s);

        if (node.inOpen && node.g <= ng) { return; }
        //printf("\nPush%6d%6d\n", s.x, s.y);

        if (!node.inOpen)
        {
            m_open.push_back(&node);
        }
        node = Node(s, p, a, ng, nh);
        node.inOpen = true;
    }

    std::vector<State> getStates() const
    {
        return {};
    }

    size_t size() const
    {
        return m_open.size();
    }

    void clear()
    {
        m_nodes.clear(Node());
        m_open.clear();
    }
};

class OpenListMapDLList : public OpenList
{
    struct NodeNode
    {
        Node node;
        NodeNode* next = nullptr;
        NodeNode* prev = nullptr;
        bool inOpen = false;
    };

    Grid<NodeNode> m_nodes;
    NodeNode* m_head = nullptr;
    size_t m_listSize = 0;

public:

    OpenListMapDLList(const Environment& env)
        : m_nodes(env.width(), env.height(), NodeNode())
    {

    }

    Node* pop()
    {
        // the min element will always be the head of the list
        Node* min = &m_head->node;
        m_head->inOpen = false;

        // the head is the new 2nd node
        m_head = m_head->next;

        m_listSize--;
        
        return min;
    }

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        NodeNode* nn = &m_nodes.get(s);
        //printf("\nInserting %d %d %d %d %d\n", s.x, s.y, ng + nh, ng, nh);

        if (s.x == 59 && s.y == 11)
        {
            int a = 6;
        }

        // if we currently have a better path in the optn list don't add this node
        if (nn->inOpen && nn->node.g <= ng) { return; }
        
        // if we are going to overwrite something in the open list, delete it first
        if (nn->inOpen)
        {
            if (nn->prev) { nn->prev->next = nn->next; }
            if (nn->next) { nn->next->prev = nn->prev; }
            m_listSize--;
        }

        // this node will now be in the open list
        nn->inOpen = true;

        // if there's no head, then this is the new head
        if (!m_head)
        {
            m_head = nn;
            nn->prev = nullptr;
            nn->next = nullptr;
        }
        else
        {
            NodeNode* current = m_head;
            int32_t nf = ng + nh;

            // find the node to insert after
            while (true)
            {
                // if the node we are looking at has a worse f, insert right before it
                if (current->node.f >= nf)
                {
                    if (current->prev) 
                    { 
                        current->prev->next = nn; 
                        nn->prev = current->prev;
                    }
                    
                    current->prev = nn;
                    nn->next = current;
                    if (current == m_head) 
                    { 
                        m_head = nn; 
                    }
                    break;
                }
                else
                {
                    if (current->next) 
                    { 
                        current = current->next;
                    }
                    else
                    {
                        current->next = nn;
                        nn->prev = current;
                        nn->next = nullptr;
                        break;
                    }
                }
            }
        }

        m_listSize++;
        nn->node = Node(s, p, a, ng, nh);
        printList();
    }

    void printList()
    {
        return;
        auto current = m_head;

        std::cout << "\n" << m_listSize << "\n";
        while (current)
        {
            std::cout << "Node ";
            current->node.print();
            current = current->next;
        }
    }

    std::vector<State> getStates() const
    {
        return {};
    }

    size_t size() const
    {
        return m_listSize;
    }

    void clear()
    {
        m_nodes.clear(NodeNode());
        m_listSize = 0;
        m_head = nullptr;
    }
};




class OpenListPQ2 : public OpenList
{
    std::vector<Node*> m_container;
    std::priority_queue<Node*, std::vector<Node*>, MinFMinHPQ> m_open;
    std::vector<Node> m_allNodes;
    Grid<Node> m_nodes;

public:

    OpenListPQ2(const Environment& env)
        : m_nodes(env.width(), env.height(), Node())
        , m_open(m_container.begin(), m_container.end())
    {
        m_container.reserve(1000000);
        m_allNodes.reserve(1000000);

        //m_open = std::priority_queue<Node*, std::vector<Node*>, MinFMinHPQ>(m_container.begin(), m_container.end());
    }

    Node* pop()
    {
        Node* n = m_open.top();
        m_open.pop();
        return n;
    }

    ~OpenListPQ2()
    {
        clear();
    }

    void push(State s, Node* p, Action a, int32_t ng, int32_t nh)
    {
        auto& node = m_nodes.get(s);
        if (node.isValid && node.g <= ng) { return; }
        m_allNodes.push_back(Node(s, p, a, ng, nh));
        m_open.push(&m_allNodes.back());
        node = m_allNodes.back();
    }

    std::vector<State> getStates() const
    {
        return {};
    }

    size_t size() const
    {
        return m_open.size();
    }

    void clear()
    {
        m_allNodes.clear();
        m_open = {};
        m_nodes.clear(Node());
    }
};
