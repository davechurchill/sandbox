#pragma once

#include "ParticleManager.h"
#include "Profiler.hpp"
#include "TopographyVisualizer.hpp"

class Visualizer_Vectors : public TopographyVisualizer
{
    cv::Mat             m_cvTransformedDepthImage32f;
    sf::Image           m_sfTransformedDepthImage;
    sf::Texture         m_sfTransformedDepthTexture;
    sf::Sprite          m_sfTransformedDepthSprite{ m_sfTransformedDepthTexture };
    sf::Shader          m_shader;
    ParticleManager     m_particleManager{};
    SandBoxProjector & activeProjector();
    void imguiControls();
    void updateParticles(const TerrainFrame & data);
    void renderVectors(sf::RenderWindow & window);

public:
    static constexpr std::string_view Name = "Vectors";
    Visualizer_Vectors() : TopographyVisualizer(Name) {}

    bool usesCanvasInput() const override { return false; }
    void init() override;
    void imgui() override;
    void process(const TerrainFrame & data) override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
