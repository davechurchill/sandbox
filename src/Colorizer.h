#pragma once

#include "TopographyProcessor.h"
#include "SandboxProjector.h"

class Colorizer : public TopographyProcessor 
{
    SandBoxProjector m_projector;
    cv::Mat m_cvTransformedDepthImage32f;
    bool m_draw;
    sf::Image m_sfTransformedDepthImage;
    sf::Texture m_sfTransformedDepthTexture;
    sf::Sprite m_sfTransformedDepthSprite;
    sf::Shader          m_shader;
    int                 m_selectedShaderIndex = 0;
    bool                m_drawContours = true;
    int                 m_numberOfContourLines = 15;

public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(std::ofstream & fout);
    void load(const std::string & fileName);

    void processTopography(const cv::Mat & data);
};