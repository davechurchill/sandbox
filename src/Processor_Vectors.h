#pragma once

#include "ParticleManager.h"
#include "Profiler.hpp"
#include "SandboxProjector.h"
#include "TopographyOverlay.hpp"
#include "TopographyProcessor.hpp"

class Processor_Vectors : public TopographyProcessor, public TopographyOverlay
{
    SandBoxProjector    m_projector;
    cv::Mat             m_cvTransformedDepthImage32f;
    sf::Image           m_sfTransformedDepthImage;
    sf::Texture         m_sfTransformedDepthTexture;
    sf::Sprite          m_sfTransformedDepthSprite{ m_sfTransformedDepthTexture };
    sf::Shader          m_shader;
    int                 m_selectedShaderIndex = 0;
    bool                m_drawContours = true;
    int                 m_numberOfContourLines = 19;
    ParticleManager     m_particleManager{};
    TopographyProcessor * m_overlayProcessor = nullptr;

    static const char* m_algorithms[];
    static const char* m_shaders[];

    SandBoxProjector & activeProjector();
    void imguiControls(bool overlayOnly);
    void renderVectors(sf::RenderWindow & window, bool overlayOnly);

public:
    void init();
    void imgui();
    void render(sf::RenderWindow& window);
    void processEvent(const sf::Event& event, const sf::Vector2f& mouse);
    void save(Save& save) const;
    void load(const Save& save);
    SandBoxProjector & projector() { return m_projector; }

    void processTopography(const IntermediateData& data);

    bool usesCanvasInput() const override { return false; }
    void initOverlay();
    void imguiOverlay();
    void processTopographyOverlay(const IntermediateData & data, TopographyProcessor & processor);
    void renderOverlay(sf::RenderWindow & window, TopographyProcessor & processor);
    void processOverlayEvent(
        const sf::Event & event,
        const sf::Vector2f & mouse,
        TopographyProcessor & processor);
    void saveOverlay(Save & save) const;
    void loadOverlay(const Save & save);
};
