#pragma once

#include "Profiler.hpp"
#include "ProjectedSurface.h"
#include "TopographyProcessor.hpp"

class Processor_Colorizer : public TopographyProcessor 
{
    ProjectedSurface    m_surface;
    sf::Shader          m_shader;
    int                 m_selectedShaderIndex = 0;
    bool                m_drawContours = true;
    int                 m_numberOfContourLines = 19;

public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);
    void processTopography(const IntermediateData& data);
};
