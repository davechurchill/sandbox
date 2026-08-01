#pragma once

#include "ProjectedSurface.h"
#include "TopographyProcessor.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Processor_Nature : public TopographyProcessor
{
    cv::Mat m_topography;
    cv::Size m_topographySize;

    ProjectedSurface m_surface;
    sf::Shader m_shader;

    float m_waterLevel = 0.28f;
    int m_terrainType = 0;
    bool m_hasFrame = false;
    bool m_shaderLoaded = false;

    void reloadShader();

public:
    void init() override;
    void imgui() override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Save & save) const override;
    void load(const Save & save) override;
    bool isTerrainWalkable(
        const cv::Mat & terrain,
        const cv::Point2f & position) const override;
    void processTopography(const IntermediateData & data) override;
};
