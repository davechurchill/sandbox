#pragma once

#include "Save.hpp"
#include "MarkerData.hpp"
#include "SandboxProjector.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>

struct IntermediateData
{
    cv::Mat topography;
    float deltaTime;
    std::vector<MarkerData> markers;
};

class TopographyProcessor
{
public:
    virtual ~TopographyProcessor() = default;

    virtual void init() = 0;
    virtual void imgui() = 0;
    virtual void render(sf::RenderWindow& window) = 0;
    virtual void processEvent(const sf::Event& event, const sf::Vector2f& mouse) = 0;
    virtual void save(Save& save) const = 0;
    virtual void load(const Save& save) = 0;
    virtual SandBoxProjector & projector() = 0;

    virtual void onSourceChanged() {}
    virtual bool usesCanvasInput() const { return false; }

    virtual bool isTerrainWalkable(const cv::Mat & terrain, const cv::Point2f & position) const
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

    virtual void processTopography(const IntermediateData& data) = 0;
};
