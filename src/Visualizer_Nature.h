#pragma once

#include "Visualizer.hpp"

#include <SFML/Graphics.hpp>

class Visualizer_Nature : public Visualizer
{
    sf::Shader m_shader;

    float m_waterLevel = 0.28f;
    int m_terrainType = 0;
    bool m_textureDetail = true;
    bool m_shaderLoaded = false;

    void reloadShader();

public:
    static constexpr std::string_view Name = "Nature";
    explicit Visualizer_Nature(SandboxProjector & projector) : Visualizer(Name, projector) {}

    void init() override;
    void imgui() override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
