#pragma once

#include "Save.hpp"
#include "MarkerData.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

struct IntermediateData
{
    cv::Mat topography;
    float deltaTime;
    std::vector<MarkerData> markers;
};

class TopographyProcessor
{
public:
    virtual void init() = 0;
    virtual void imgui() = 0;
    virtual void render(sf::RenderWindow& window) = 0;
    virtual void processEvent(const sf::Event& event, const sf::Vector2f& mouse) = 0;
    virtual void save(Save& save) const = 0;
    virtual void load(const Save& save) = 0;

    virtual void processTopography(const IntermediateData& data) = 0;
};
