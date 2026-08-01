#pragma once

#include "TopographyProcessor.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Processor_Blockworld : public TopographyProcessor
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
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Settings & save) const;
    void load(const Settings & save);
    void processTopography(const IntermediateData & data);
};
