#pragma once

#include "TopographyOverlay.hpp"

class Overlay_ColorAdjustment final : public TopographyOverlay
{
    cv::Mat m_projectedTopography;
    sf::Image m_terrainImage;
    sf::Texture m_terrainTexture;
    sf::Sprite m_terrainSprite{ m_terrainTexture };
    sf::Texture m_captureTexture;
    sf::Sprite m_captureSprite{ m_captureTexture };
    sf::Shader m_shader;

    float m_brightness = 0.0f;
    float m_contrast = 1.0f;
    float m_exposure = 0.0f;
    float m_saturation = 1.0f;
    float m_hue = 0.0f;
    float m_gamma = 1.0f;
    float m_temperature = 0.0f;
    bool m_hasFrame = false;
    bool m_shaderLoaded = false;

    void reloadShader();
    void resetAdjustments();
    bool hasAdjustments() const;

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
    void saveOverlay(Settings & save) const override;
    void loadOverlay(const Settings & save) override;
};
