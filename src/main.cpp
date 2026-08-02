#include "Sandbox.h"

int main()
{
    PROFILE_FUNCTION();

    cv::setNumThreads(cv::getNumberOfCPUs());

    SandboxGUI sandbox;
    sandbox.run();

    return 0;
}
