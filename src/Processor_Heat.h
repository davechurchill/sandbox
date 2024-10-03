#pragma once

#include "HeatGrid.h"
#include "Profiler.hpp"
#include "SandboxProjector.h"
#include "Tools.h"
#include "TopographyProcessor.h"

class Processor_Heat : public TopographyProcessor
{
    HeatGrid    m_heatGrid;

    SandBoxProjector m_projector;
    bool        m_drawProjection = true;

    cv::Mat     m_cvTransformedDepthImage32fColor;
    sf::Image   m_sfTransformedDepthImageColor;
    sf::Texture m_sfTransformedDepthTextureColor;
    sf::Sprite  m_sfTransformedDepthSpriteColor;
    sf::Shader  m_shader_color;

    cv::Mat     m_cvTransformedDepthImage32fHeat;
    sf::Image   m_sfTransformedDepthImageHeat;
    sf::Texture m_sfTransformedDepthTextureHeat;
    sf::Sprite  m_sfTransformedDepthSpriteHeat;
    sf::Shader  m_shader_heat;

    bool        m_drawContours = false;
    int         m_numberOfContourLines = 19;
    int         m_iterations = 0;
    bool        m_doStep = false;

    bool        m_drawingSource = false;
    cv::Point   m_sources;

    int         m_selectedSource = 0;

    sf::Vector2f m_previousMouse;

    void setInitialHeatSources();

public:
    void init();
    void imgui();
    void render(sf::RenderWindow& window);
    void processEvent(const sf::Event& event, const sf::Vector2f& mouse);
    void save(Save& save) const;
    void load(const Save& save);

    void processTopography(const cv::Mat& data);
};
