#pragma once

#include "ProjectedSurface.h"
#include "TopographyProcessor.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Processor_Blockworld : public TopographyProcessor
{
    ProjectedSurface    m_surface;
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
    void processTopography(const IntermediateData & data);
};
