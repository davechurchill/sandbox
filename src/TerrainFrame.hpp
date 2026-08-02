#pragma once

#include "MarkerDetector.hpp"

#include <opencv2/core.hpp>

#include <vector>

struct TerrainFrame
{
    const cv::Mat & heightMap;
    float deltaTime;
    const std::vector<MarkerData> & markers;
};
