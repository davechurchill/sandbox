#pragma once

#include "Visualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Visualizer_Minecraft final : public Visualizer
{
    sf::Shader m_shader;

    int m_blockSize = 24;
    int m_heightSteps = 28;
    float m_blockRelief = 1.0f;
    float m_waterLevel = 0.28f;
    float m_snowLine = 0.82f;
    float m_aoStrength = 0.85f;
    float m_time = 0.0f;
    bool m_shaderLoaded = false;

    void resetDefaults();
    void clampSettings();
    void reloadShader();

public:
    static constexpr std::string_view Name = "Minecraft";
    explicit Visualizer_Minecraft(SandBoxProjector & projector) : Visualizer(Name, projector) {}

    void init() override;
    void imgui() override;
    void process(const TerrainFrame & data) override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
