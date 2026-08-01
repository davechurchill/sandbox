#pragma once

#include "TopographyOverlay.hpp"

#include <array>

class Overlay_ContourLines final : public TopographyOverlay
{
    cv::Mat m_projectedTopography;
    sf::Image m_image;
    sf::Texture m_texture;
    sf::Sprite m_sprite{ m_texture };
    sf::Shader m_shader;

    std::array<float, 3> m_lineColor{ 0.0f, 0.0f, 0.0f };
    float m_lineOpacity = 1.0f;
    int m_numberOfContourLines = 19;
    bool m_hasFrame = false;
    bool m_shaderLoaded = false;

    void reloadShader();

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
