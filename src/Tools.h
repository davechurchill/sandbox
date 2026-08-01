#pragma once

#include <SFML/Graphics.hpp>
#include <opencv2/opencv.hpp>   // Include OpenCV API

#include <string>
#include <vector>

namespace Tools
{
    bool imguiMonitorSelector(const sf::Window & mainWindow, std::string & selectedMonitorID);
    void openDisplayWindow(sf::RenderWindow & displayWindow, const sf::Window & mainWindow, const std::string & selectedMonitorID);

    // given an (mx, my) mouse position, return the index of the first circle the contains the position
    // returns -1 if the mouse position is not inside any circle
    int getClickedCircleIndex(float mx, float my, std::vector<sf::CircleShape> & circles);

    sf::Image matToSfImage(const cv::Mat & mat);

}
