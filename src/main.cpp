#include "Sandbox.hpp"

int main()
{
    PROFILE_FUNCTION();

    cv::setNumThreads(cv::getNumberOfCPUs());

    SandboxGUI sandbox;
    sandbox.run();

    return 0;
}
