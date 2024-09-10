#include "GameEngine.h"
#include "Scene_Menu.h"
#include "Profiler.hpp"

#include <sstream>
#include <iostream>

int main()
{
    PROFILE_FUNCTION();

    GameEngine engine;
    engine.changeScene<Scene_Menu>("Menu");
    engine.run();

    return 0;
}

