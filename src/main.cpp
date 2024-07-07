#include "GameEngine.h"
#include "Scene_Menu.h"
#include "Scene_Perlin2D.h"
#include "Scene_Sandbox.h"
#include "Profiler.hpp"

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
    PROFILE_FUNCTION();

    GameEngine engine;
    engine.changeScene<Scene_Menu>("Menu");
    engine.run();

    return 0;
}

