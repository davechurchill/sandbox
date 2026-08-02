#pragma once

#include "Visualizer.hpp"

#include <SFML/Graphics.hpp>
#include <opencv2/opencv.hpp>

class Visualizer_PrismaticGlacier : public Visualizer
{
    sf::Shader m_shader;
    float m_facetSize = 18.0f;
    float m_iceDepthAbsorption = 1.15f;
    float m_iridescence = 0.80f;
    float m_fractureIntensity = 0.85f;
    float m_causticSpeed = 0.65f;
    float m_time = 0.0f;
    bool m_shaderLoaded = false;

    void resetDefaults();
    void clampSettings();
    void reloadShader();

public:
    static constexpr std::string_view Name = "Prismatic Crystal Glacier";
    explicit Visualizer_PrismaticGlacier(SandBoxProjector & projector) : Visualizer(Name, projector) {}

    void init() override;
    void imgui() override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
    void process(const TerrainFrame & data) override;
};
