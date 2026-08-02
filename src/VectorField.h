#pragma once

#include <opencv2/core.hpp>

namespace VectorField
{
    cv::Mat ComputeBFS(const cv::Mat& grid, int spacing, float heightPenalty);
}
