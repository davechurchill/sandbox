#pragma once

#include "Visualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Visualizer_Blockworld : public Visualizer
{
    cv::Mat             m_projectedTopography;
    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite{ m_texture };
    sf::Shader          m_shader;

    int                 m_blockSize = 12;
    bool                m_hasFrame = false;
    bool                m_shaderLoaded = false;

    void reloadShader();

public:
    static constexpr std::string_view Name = "Blockworld";
    explicit Visualizer_Blockworld(SandBoxProjector & projector) : Visualizer(Name, projector) {}

    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Settings & save) const;
    void load(const Settings & save);
    void process(const TerrainFrame & data) override;
};
