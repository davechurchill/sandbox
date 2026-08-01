#pragma once

#include "SandboxProjector.h"
#include "TopographyProcessor.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Processor_TerrainLighting : public TopographyProcessor
{
    SandBoxProjector    m_projector;

    cv::Mat             m_projectedTopography;
    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite{ m_texture };
    sf::Shader          m_shader;

    float               m_lightAzimuth = 315.0f;
    float               m_lightElevation = 45.0f;
    float               m_ambientLight = 0.2f;
    float               m_shadowStrength = 1.0f;
    float               m_heightStrength = 10.0f;
    int                 m_palette = 0;
    bool                m_hasFrame = false;
    bool                m_shaderLoaded = false;
    bool                m_draggingLight = false;

    void reloadShader();
    bool updateLightFromMouse(const sf::Vector2f & mouse);

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
