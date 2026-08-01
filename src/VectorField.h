#pragma once

#include <opencv2/core.hpp>

namespace VectorField
{
    cv::Mat computeBFS(const cv::Mat& grid, int spacing, float heightPenalty);
}
