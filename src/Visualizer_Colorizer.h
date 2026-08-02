#pragma once

#include "Profiler.hpp"
#include "Visualizer.hpp"

class Visualizer_Colorizer : public Visualizer
{
    sf::Shader          m_shader;
    int                 m_selectedShaderIndex = 0;

public:
    static constexpr std::string_view Name = "Colorizer";
    explicit Visualizer_Colorizer(SandboxProjector & projector) : Visualizer(Name, projector) {}

    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Settings & save) const;
    void load(const Settings & save);
};
