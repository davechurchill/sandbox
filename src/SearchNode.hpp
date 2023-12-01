#pragma once

#include "Action.hpp"
#include "Grid.hpp"

#include <memory>

class Node
{
public:
    State    state;
    Action  action;
    Node*   parent = nullptr;
    int32_t f = 0;
    int32_t g = -1;
    int32_t h = 0;
    int inOpen = 0;
    bool isValid = false;

    Node() {}

    Node(State s, Node* p, Action a, int32_t ng, int32_t nh)
        : state(s), parent(p), action(a), g(ng), h(nh), f(ng+nh), isValid(true)
    {
        
    }

    void print()
    {
        printf("Node%5d%5d%5d%5d%5d\n", state.x, state.y, f, g, h);
    }
};

struct MinFMinG
{
    bool operator()(const Node* n1, const Node* n2) const
    {
        return (n1->f == n2->f) ? (n1->g < n2->g) : (n1->f < n2->f);
    }
};

struct MinFMinH
{
    bool operator()(const Node* n1, const Node* n2) const
    {
        return (n1->f == n2->f) ? (n1->h < n2->h) : (n1->f < n2->f);
    }
};

struct MinFMinHPQ
{
    bool operator()(const Node* n1, const Node* n2) const
    {
        return !MinFMinH()(n1, n2);
    }
};
