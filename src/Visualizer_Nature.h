#pragma once

#include "TopographyVisualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Visualizer_Nature : public TopographyVisualizer
{
    cv::Mat m_topography;
    cv::Mat m_projectedTopography;
    cv::Size m_topographySize;

    sf::Image m_image;
    sf::Texture m_texture;
    sf::Sprite m_sprite{ m_texture };
    sf::Shader m_shader;

    float m_waterLevel = 0.28f;
    int m_terrainType = 0;
    bool m_hasFrame = false;
    bool m_shaderLoaded = false;

    void reloadShader();

public:
    static constexpr std::string_view Name = "Nature";
    Visualizer_Nature() : TopographyVisualizer(Name) {}

    void init() override;
    void imgui() override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
    void process(const TerrainFrame & data) override;
};
