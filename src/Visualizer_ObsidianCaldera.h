#pragma once

#include "Visualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Visualizer_ObsidianCaldera : public Visualizer
{
    cv::Mat m_projectedTopography;
    sf::Image m_image;
    sf::Texture m_texture;
    sf::Sprite m_sprite{ m_texture };
    sf::Shader m_shader;
    sf::Clock m_clock;

    float m_lavaLevel = 0.28f;
    float m_crackIntensity = 1.25f;
    float m_crackScale = 34.0f;
    float m_flowSpeed = 0.75f;
    float m_cooling = 0.42f;
    float m_heatDistortion = 0.35f;
    bool m_hasFrame = false;
    bool m_shaderLoaded = false;

    void reloadShader();
    void resetDefaults();

public:
    static constexpr std::string_view Name = "Obsidian Caldera";
    explicit Visualizer_ObsidianCaldera(SandBoxProjector & projector) : Visualizer(Name, projector) {}

    void init() override;
    void imgui() override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
    void process(const TerrainFrame & data) override;
};
