#include "GameEngine.h"
#include "Scene_Menu.h"
#include "Scene_Perlin2D.h"
#include "Scene_Sandbox.h"
#include "Profiler.hpp"

#include <sstream>
#include <iostream>

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp> // Include OpenCV API
#include <SFML/Graphics.hpp> // Include SFML Graphics API

sf::Image matToSfImage(const cv::Mat& mat) {
    // Ensure the input image is in the correct format (CV_32F)
    cv::Mat normalized;
    mat.convertTo(normalized, CV_8U, 255.0); // Scale float [0, 1] to [0, 255]

    // Convert to RGB (SFML requires RGB format)
    cv::Mat rgb;
    cv::cvtColor(normalized, rgb, cv::COLOR_GRAY2RGBA);

    // Create SFML image
    sf::Image image;
    image.create(rgb.cols, rgb.rows, rgb.ptr());

    return image;
}

int main2() {
    // Initialize RealSense pipeline
    rs2::pipeline p;
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH, RS2_FORMAT_Z16);

    // Start streaming with the configured settings
    p.start(cfg);

    // Create the SFML window
    sf::RenderWindow window(sf::VideoMode(640, 480), "Depth Image");

    while (window.isOpen()) {
        // Wait for the next set of frames
        rs2::frameset frames = p.wait_for_frames();

        // Get the depth frame
        rs2::depth_frame depthFrame = frames.get_depth_frame();

        // Convert the depth frame to a cv::Mat
        cv::Mat depthImage16u(cv::Size(depthFrame.get_width(), depthFrame.get_height()), CV_16U, (void*)depthFrame.get_data(), cv::Mat::AUTO_STEP);

        // Convert depth data to float and normalize to [0, 1]
        cv::Mat depthImageFloat;
        depthImage16u.convertTo(depthImageFloat, CV_32F);

        // Optionally, normalize depth values
        double minVal, maxVal;
        cv::minMaxLoc(depthImageFloat, &minVal, &maxVal);
        depthImageFloat = depthImageFloat / maxVal;

        // Convert the normalized depth image to SFML image
        sf::Image sfImage = matToSfImage(depthImageFloat);

        // Create SFML texture and sprite
        sf::Texture texture;
        if (!texture.loadFromImage(sfImage)) {
            return -1; // Handle loading error
        }
        sf::Sprite sprite;
        sprite.setTexture(texture);

        // Handle SFML window events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Clear the window
        window.clear();

        // Draw the sprite
        window.draw(sprite);

        // Display the window
        window.display();

        // Also show the image using OpenCV's imshow
        cv::imshow("Depth Image (OpenCV)", depthImageFloat);
        if (cv::waitKey(1) == 27) { // Exit on 'ESC' key press
            break;
        }
    }

    return 0;
}


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

