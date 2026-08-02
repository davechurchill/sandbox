#pragma once

#include "Visualizer.hpp"

class Visualizer_ColorAdjustment final : public Visualizer
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
    static constexpr std::string_view Name = "Adjust Terrain Color";
    explicit Visualizer_ColorAdjustment(SandBoxProjector & projector) : Visualizer(Name, projector) {}

    void init() override;
    void imgui() override;
    void process(const TerrainFrame & data) override;
    void render(sf::RenderWindow & window) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
