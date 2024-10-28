#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"

class HandDetection
{
    int m_thresh = 100;
    sf::Image image;
    sf::Texture tex;
    int label = 0;
public:
    void imgui(const cv::Mat & input);
};