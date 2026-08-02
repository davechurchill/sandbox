#pragma once

#include "Visualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Visualizer_Hexagon final : public Visualizer
{
    sf::Shader m_shader;

    int m_hexagonSize = 18;
    int m_heightSteps = 28;
    float m_prismRelief = 1.15f;
    bool m_shaderLoaded = false;

    void resetDefaults();
    void clampSettings();
    void reloadShader();

public:
    static constexpr std::string_view Name = "Hexagon";
    explicit Visualizer_Hexagon(SandboxProjector & projector) : Visualizer(Name, projector) {}

    void init() override;
    void imgui() override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
