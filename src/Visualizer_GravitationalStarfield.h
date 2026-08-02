#pragma once

#include "Visualizer.hpp"

#include <opencv2/core.hpp>
#include <SFML/Graphics.hpp>

class Visualizer_GravitationalStarfield final : public Visualizer
{
    sf::Shader m_shader;

    float m_time = 0.0f;
    float m_lensingStrength = 1.25f;
    float m_starDensity = 58.0f;
    float m_nebulaIntensity = 0.85f;
    float m_ringIntensity = 1.35f;
    float m_driftSpeed = 0.22f;
    bool m_shaderLoaded = false;

    void reloadShader();
    void resetDefaults();

public:
    static constexpr std::string_view Name = "Gravitational Starfield";
    explicit Visualizer_GravitationalStarfield(SandboxProjector & projector) : Visualizer(Name, projector) {}

    void init() override;
    void imgui() override;
    void process(const TerrainFrame & data) override;
    void render(sf::RenderWindow & window) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
