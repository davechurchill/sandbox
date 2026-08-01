#pragma once

#include <opencv2/core.hpp>

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

class SandBoxProjector;

class TerrainContext
{
public:
    using WalkabilityFunction = std::function<bool(
        const cv::Mat &,
        const cv::Point2f &)>;

private:
    SandBoxProjector & m_projector;
    WalkabilityFunction m_walkabilityFunction;

public:
    explicit TerrainContext(SandBoxProjector & projector)
        : m_projector(projector)
    {
    }

    void setWalkabilityProvider(WalkabilityFunction function)
    {
        m_walkabilityFunction = std::move(function);
    }

    SandBoxProjector & projector() const { return m_projector; }

    static bool defaultTerrainWalkability(
        const cv::Mat & terrain,
        const cv::Point2f & position)
    {
        if (terrain.empty() || terrain.type() != CV_32F
            || position.x < 0.0f || position.y < 0.0f
            || position.x >= terrain.cols || position.y >= terrain.rows)
        {
            return false;
        }

        const int x = std::clamp((int)std::round(position.x), 0, terrain.cols - 1);
        const int y = std::clamp((int)std::round(position.y), 0, terrain.rows - 1);
        const float height = terrain.at<float>(y, x);
        return std::isfinite(height) && height > 0.001f && height < 0.999f;
    }

    bool isTerrainWalkable(
        const cv::Mat & terrain,
        const cv::Point2f & position) const
    {
        return m_walkabilityFunction
            ? m_walkabilityFunction(terrain, position)
            : defaultTerrainWalkability(terrain, position);
    }
};
