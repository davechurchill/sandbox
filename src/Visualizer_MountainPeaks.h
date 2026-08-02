#pragma once

#include "Visualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Visualizer_MountainPeaks final : public Visualizer
{
    sf::Shader m_shader;

    float m_time = 0.0f;
    float m_snowLine = 0.58f;
    float m_snowCoverage = 1.0f;
    float m_rockContrast = 1.2f;
    float m_sunAzimuth = 315.0f;
    float m_sunElevation = 38.0f;
    float m_haze = 0.32f;
    bool m_shaderLoaded = false;

    void resetDefaults();
    void clampSettings();
    void reloadShader();

public:
    static constexpr std::string_view Name = "Mountain Peaks";
    explicit Visualizer_MountainPeaks(SandBoxProjector & projector) : Visualizer(Name, projector) {}

    void init() override;
    void imgui() override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
    void process(const TerrainFrame & data) override;
};
