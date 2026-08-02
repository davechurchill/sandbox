#pragma once

#include "HeatGrid.h"
#include "Profiler.hpp"
#include "Tools.h"
#include "Visualizer.hpp"

class Visualizer_Heat : public Visualizer
{
    HeatGrid    m_heatGrid;

    bool        m_drawProjection = true;

    cv::Mat     m_cvTransformedDepthImage32fColor;
    sf::Image   m_sfTransformedDepthImageColor;
    sf::Texture m_sfTransformedDepthTextureColor;
    sf::Sprite  m_sfTransformedDepthSpriteColor{ m_sfTransformedDepthTextureColor };
    sf::Shader  m_shader_color;

    cv::Mat     m_cvTransformedDepthImage32fHeat;
    sf::Image   m_sfTransformedDepthImageHeat;
    sf::Texture m_sfTransformedDepthTextureHeat;
    sf::Sprite  m_sfTransformedDepthSpriteHeat{ m_sfTransformedDepthTextureHeat };
    sf::Shader  m_shader_heat;

    int         m_iterations = 0;
    bool        m_doStep = false;

    int         m_selectedSource = 0;

    sf::Vector2f m_previousMouse;

    void setInitialHeatSources();

public:
    static constexpr std::string_view Name = "Heat";
    explicit Visualizer_Heat(SandboxProjector & projector) : Visualizer(Name, projector) {}

    void init();
    void imgui();
    void render(sf::RenderWindow& window);
    void processEvent(const sf::Event& event, const sf::Vector2f& mouse);
    void save(Settings& save) const;
    void load(const Settings& save);
    void process(const TerrainFrame& data) override;
};
