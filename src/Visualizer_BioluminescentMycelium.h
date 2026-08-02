#pragma once

#include "Visualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Visualizer_BioluminescentMycelium : public Visualizer
{
    cv::Mat m_projectedTopography;
    sf::Image m_image;
    sf::Texture m_texture;
    sf::Sprite m_sprite{ m_texture };
    sf::Shader m_shader;

    float m_glowIntensity = 1.35f;
    float m_networkScale = 8.0f;
    float m_pulseSpeed = 1.0f;
    float m_sporeDensity = 0.35f;
    float m_time = 0.0f;
    bool m_hasFrame = false;
    bool m_shaderLoaded = false;

    void reloadShader();
    void resetDefaults();

public:
    static constexpr std::string_view Name = "Bioluminescent Mycelium";
    explicit Visualizer_BioluminescentMycelium(SandBoxProjector & projector) : Visualizer(Name, projector) {}

    void init() override;
    void imgui() override;
    void process(const TerrainFrame & data) override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
