#pragma once

#include "Visualizer.hpp"

#include <array>

class Visualizer_ContourLines final : public Visualizer
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
    static constexpr std::string_view Name = "Contour Lines";
    explicit Visualizer_ContourLines(SandBoxProjector & projector) : Visualizer(Name, projector) {}

    void init() override;
    void imgui() override;
    void process(const TerrainFrame & data) override;
    void render(sf::RenderWindow & window) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
