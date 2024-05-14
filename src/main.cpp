#include "GameEngine.h"
#include "Scene_Grid.h"
#include "Scene_Perlin2D.h"
#include "Scene_Sandbox.h"

#include <sstream>
#include <iostream>

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

bool isCameraConnected() {
    rs2::context ctx;  // Create a context object, which is used to manage devices
    rs2::device_list devices = ctx.query_devices();  // Get a list of connected RealSense devices
    return devices.size() > 0;  // Return true if at least one device is connected
}

int main()
{
    if (!isCameraConnected())
    {
        std::cerr << "No RealSense Camera Found, check connection and restart\n";
        return -1;
    }


    GameEngine engine;
    //engine.changeScene("Grid", std::make_shared<Scene_Grid>(&engine));
    //engine.changeScene("Perlin", std::make_shared<Scene_Perlin2D>(&engine));
    engine.changeScene("Sandbox", std::make_shared<Scene_Sandbox>(&engine));
    engine.run();

    return 0;
}

