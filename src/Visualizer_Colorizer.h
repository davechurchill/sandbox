#pragma once

#include "Profiler.hpp"
#include "Visualizer.hpp"

class Visualizer_Colorizer : public Visualizer
{
    cv::Mat             m_projectedTopography;
    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite{ m_texture };
    sf::Shader          m_shader;
    int                 m_selectedShaderIndex = 0;
    bool                m_drawContours = true;
    int                 m_numberOfContourLines = 19;

public:
    static constexpr std::string_view Name = "Colorizer";
    explicit Visualizer_Colorizer(SandBoxProjector & projector) : Visualizer(Name, projector) {}

    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Settings & save) const;
    void load(const Settings & save);
    void process(const TerrainFrame& data) override;
};
