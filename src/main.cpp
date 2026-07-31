#include "Sandbox.h"

int main()
{
    PROFILE_FUNCTION();

    cv::setNumThreads(cv::getNumberOfCPUs());

    GameEngine engine;
    engine.changeScene<Scene_Main>("Menu");
    engine.run();

    return 0;
}

