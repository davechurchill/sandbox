#pragma once

#include <opencv2/opencv.hpp>

class HandDetection
{
    int m_thresh = 100;
public:
    void imgui(const cv::Mat & input);
};