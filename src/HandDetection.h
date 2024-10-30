#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"

class HandDetection
{
    int m_thresh = 218;
    bool m_useInput = true;
    bool m_usePrevious = true;
    sf::Image m_image;
    sf::Texture m_texture;

    cv::Mat m_previous;
    cv::Mat m_segmented;
public:
    void imgui();
    void removeHands(const cv::Mat & input, cv::Mat & output, float maxDistance, float minDistance);
};