#pragma once

#include "ParticleManager.h"
#include "Profiler.hpp"
#include "TopographyOverlay.hpp"

class Overlay_Vectors : public TopographyOverlay
{
    cv::Mat             m_cvTransformedDepthImage32f;
    sf::Image           m_sfTransformedDepthImage;
    sf::Texture         m_sfTransformedDepthTexture;
    sf::Sprite          m_sfTransformedDepthSprite{ m_sfTransformedDepthTexture };
    sf::Shader          m_shader;
    ParticleManager     m_particleManager{};
    TopographyProcessor * m_overlayProcessor = nullptr;

    SandBoxProjector & activeProjector();
    void imguiControls();
    void updateParticles(const IntermediateData & data);
    void renderVectors(sf::RenderWindow & window);

public:
    bool usesCanvasInput() const override { return false; }
    void initOverlay() override;
    void imguiOverlay() override;
    void processTopographyOverlay(
        const IntermediateData & data,
        TopographyProcessor & processor) override;
    void renderOverlay(
        sf::RenderWindow & window,
        TopographyProcessor & processor) override;
    void processOverlayEvent(
        const sf::Event & event,
        const sf::Vector2f & mouse,
        TopographyProcessor & processor) override;
    void saveOverlay(Save & save) const override;
    void loadOverlay(const Save & save) override;
};
