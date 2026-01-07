# MUN AI & Games Lab Augmented Reality Sandbox

![sandbox](https://davechurchill.ca/files/images/sandbox/sandbox_github.jpg)

[Sandbox Image Gallery](https://davechurchill.ca/files/images/sandbox/)

# Hardware and Software

The hardware used for our sandbox is as follows:

- Any computer with a decent CPU / GPU
- Intel RealSense Depth Camera D455
- BenQ 1080p DLP Gaming Projector TH575

Our software is written in C++ and uses the following external libraries:

- SFML 2.6.2
- Realsense SDK
- OpenCV 4.8.0

# Installation

This program uses sfml, the realsense sdk, and opencv, which must be installed separately.

For sfml you must create an environment variable called SFML_DIR pointing towards the sfml directory.

For opencv you must use opencv 4.8.0, and add the opencv\build\x64\vc16\bin directory to the environment path variable.

For the realsense sdk, simply downloading and installing it should suffice.
