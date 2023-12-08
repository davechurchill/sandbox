#include "GameEngine.h"
#include "Scene_Grid.h"
#include "Scene_Perlin2D.h"
#include "Scene_Sandbox.h"

#include <sstream>
#include <iostream>

int main()
{
    GameEngine engine;
    //engine.changeScene("Grid", std::make_shared<Scene_Grid>(&engine));
    //engine.changeScene("Perlin", std::make_shared<Scene_Perlin2D>(&engine));
    engine.changeScene("Sandbox", std::make_shared<Scene_Sandbox>(&engine));
    engine.run();

    return 0;
}

