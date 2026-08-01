#pragma once

#include "SandboxProjector.h"
#include "TopographyProcessor.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Processor_Minecraft : public TopographyProcessor
{
    SandBoxProjector    m_projector;

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
    void save(Save & save) const;
    void load(const Save & save);
    SandBoxProjector & projector() { return m_projector; }

    void processTopography(const IntermediateData & data);
};
