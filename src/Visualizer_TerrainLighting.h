#pragma once

#include "Visualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Visualizer_TerrainLighting : public Visualizer
{
    sf::Shader          m_shader;

    float               m_lightAzimuth = 315.0f;
    float               m_lightElevation = 45.0f;
    float               m_ambientLight = 0.2f;
    float               m_shadowStrength = 1.0f;
    float               m_heightStrength = 10.0f;
    int                 m_palette = 0;
    bool                m_shaderLoaded = false;
    bool                m_draggingLight = false;

    void reloadShader();
    bool updateLightFromMouse(const sf::Vector2f & mouse);

public:
    static constexpr std::string_view Name = "TerrainLighting";
    explicit Visualizer_TerrainLighting(SandBoxProjector & projector) : Visualizer(Name, projector) {}

    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Settings & save) const;
    void load(const Settings & save);
};
