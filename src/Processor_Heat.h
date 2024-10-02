#pragma once

#include "HeatMap.h"
#include "Profiler.hpp"
#include "SandboxProjector.h"
#include "Tools.h"
#include "TopographyProcessor.h"

namespace
{
    constexpr void drawSquare(std::vector<HeatMap::HeatSource>& sources, int x, int y, int w, int h)
    {
        sources.push_back({ { x, y }, { w, h }, 100.f });
    }

    constexpr std::vector<HeatMap::HeatSource> initialSources()
    {
        std::vector<HeatMap::HeatSource> sources{};

        drawSquare(sources, 100, 100, 20, 20);
        drawSquare(sources, 200, 100, 25, 20);
        drawSquare(sources, 200, 300, 25, 20);
        drawSquare(sources, 100, 300, 20, 20);

        return sources;
    }
}

class Processor_Heat : public TopographyProcessor
{
    HeatMap::Grid heatGrid{ initialSources() };

    SandBoxProjector m_projector;
    bool m_drawProjection = true;

    cv::Mat m_cvTransformedDepthImage32fColor;
    sf::Image m_sfTransformedDepthImageColor;
    sf::Texture m_sfTransformedDepthTextureColor;
    sf::Sprite m_sfTransformedDepthSpriteColor;
    sf::Shader          m_shader_color;

    cv::Mat m_cvTransformedDepthImage32fHeat;
    sf::Image m_sfTransformedDepthImageHeat;
    sf::Texture m_sfTransformedDepthTextureHeat;
    sf::Sprite m_sfTransformedDepthSpriteHeat;
    sf::Shader          m_shader_heat;

    bool                m_drawContours = false;
    int                 m_numberOfContourLines = 19;

    bool drawingSource = false;
    cv::Point rectStart{};

public:
    void init();
    void imgui();
    void render(sf::RenderWindow& window);
    void processEvent(const sf::Event& event, const sf::Vector2f& mouse);
    void save(Save& save) const;
    void load(const Save& save);

    void processTopography(const cv::Mat& data);
};
