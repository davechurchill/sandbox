#pragma once

#include "MarkerDetector.hpp"

#include <opencv2/core.hpp>

#include <cstdint>
#include <vector>

struct TerrainFrame
{
    const cv::Mat & heightMap;
    float deltaTime;
    const std::vector<MarkerData> & markers;
    std::uint64_t revision;
};
