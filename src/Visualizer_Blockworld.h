#pragma once

#include "Visualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Visualizer_Blockworld : public Visualizer
{
    sf::Shader          m_shader;

    int                 m_blockSize = 12;
    bool                m_shaderLoaded = false;

    void reloadShader();

public:
    static constexpr std::string_view Name = "Blockworld";
    explicit Visualizer_Blockworld(SandboxProjector & projector) : Visualizer(Name, projector) {}

    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Settings & save) const;
    void load(const Settings & save);
};
