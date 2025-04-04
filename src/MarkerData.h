#pragma once

#include <opencv2/opencv.hpp>

struct MarkerData
{
    int id;
    cv::Point2f center;
};