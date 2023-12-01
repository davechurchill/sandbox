#pragma once

struct State
{
    using CellType = int32_t;
    CellType x = 0;
    CellType y = 0;

    State() {}

    State(CellType ix, CellType iy)
        : x(ix), y(iy) { }

    State(uint32_t ix, uint32_t iy)
        : x((CellType)ix), y((CellType)iy) { }

    bool operator == (State rhs) const
    {
        return (x == rhs.x) && (y == rhs.y);
    }

    State operator + (State rhs) const
    {
        return State(x + rhs.x, y + rhs.y);
    }

    State absdiff(State rhs) const
    {
        return State(abs(x - rhs.x), abs(y - rhs.y));
    }
};

struct Action
{
    State dir;
    int32_t cost = 0;
    Action() {}
    Action(State d, int32_t c)
        : dir(d), cost(c) {}
};

#include <vector>
inline std::vector<Action>& Actions4()
{
    static std::vector<Action> actions =
    {
        Action(State(1, 0), 100),
        Action(State(-1, 0), 100),
        Action(State(0, 1), 100),
        Action(State(0,-1), 100)
    };

    return actions;
}

inline std::vector<Action>& Actions8()
{
    static std::vector<Action> actions =
    {
        Action(State(1, 0), 100),
        Action(State(-1, 0), 100),
        Action(State(0, 1), 100),
        Action(State(0,-1), 100),
        Action(State(1,1), 141),
        Action(State(1,-1), 141),
        Action(State(-1,1), 141),
        Action(State(-1,-1), 141)
    };

    return actions;
}