#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include "TopographySource.h"

class HandDetection
{
    int m_thresh = 218;
    int m_selectedHull = -1;
    sf::Image m_image;
    sf::Texture m_texture;

    cv::Mat m_previous;
    cv::Mat m_segmented;

    std::vector<std::vector<cv::Point>> m_hulls;
    std::vector<std::vector<cv::Point>> m_contours;

    std::vector<std::vector<double>> m_features;

public:
    std::vector<Gesture> m_gestures;

    void imgui();
    void removeHands(const cv::Mat & input, cv::Mat & output, float maxDistance, float minDistance);
    void identifyGestures(std::vector<cv::Point> & nbox);
};