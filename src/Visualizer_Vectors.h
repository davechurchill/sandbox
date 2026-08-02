#pragma once

#include "ParticleManager.h"
#include "Profiler.hpp"
#include "Visualizer.hpp"

class Visualizer_Vectors : public Visualizer
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
    explicit Visualizer_Vectors(SandBoxProjector & projector) : Visualizer(Name, projector) {}

    void init() override;
    void imgui() override;
    void process(const TerrainFrame & data) override;
    void render(sf::RenderWindow & window) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
