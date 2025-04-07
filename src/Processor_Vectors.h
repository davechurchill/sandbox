#pragma once

#include "ParticleManager.h"
#include "Profiler.hpp"
#include "SandboxProjector.h"
#include "TopographyProcessor.h"

class Processor_Vectors : public TopographyProcessor
{
    SandBoxProjector    m_projector;
    cv::Mat             m_cvTransformedDepthImage32f;
    sf::Image           m_sfTransformedDepthImage;
    sf::Texture         m_sfTransformedDepthTexture;
    sf::Sprite          m_sfTransformedDepthSprite;
    sf::Shader          m_shader;
    int                 m_selectedAlgorithmIndex = 0;
    int                 m_selectedShaderIndex = 0;
    bool                m_drawContours = true;
    int                 m_numberOfContourLines = 19;
    ParticleManager     m_particleManager{};

    static const char* m_algorithms[];
    static const char* m_shaders[];

public:
    void init();
    void imgui();
    void render(sf::RenderWindow& window);
    void processEvent(const sf::Event& event, const sf::Vector2f& mouse);
    void save(Save& save) const;
    void load(const Save& save);

    void processTopography(const IntermediateData& data);
};
