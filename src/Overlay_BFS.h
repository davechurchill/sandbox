#pragma once

#include "TopographyOverlay.hpp"

#include <opencv2/core.hpp>
#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

class Overlay_BFS final : public TopographyOverlay
{
    struct Particle
    {
        sf::Vector2<double> position{ 0.0, 0.0 };
        std::vector<sf::Vector2<double>> trail;
    };

    std::vector<Particle> m_particles;
    std::mt19937 m_random{ std::random_device{}() };
    cv::Size m_topographySize;

    sf::Texture m_texture;
    sf::Sprite m_sprite{ m_texture };
    sf::Shader m_shader;

    int m_trailLength = 4;
    int m_cellSize = 8;
    float m_spawnRate = 5000.0f;
    float m_particleSpeed = 120.0f;
    float m_particleAlpha = 0.85f;
    float m_heightPenalty = 0.20f;
    double m_spawnAccumulator = 0.0;
    bool m_resetRequested = true;

    void resetParticles();
    void updateParticles(const cv::Mat & terrain, float deltaTime);
    void updateTexture(const cv::Mat & terrain, TopographyProcessor & processor);

public:
    bool usesCanvasInput() const override { return false; }
    void initOverlay() override;
    void imguiOverlay() override;
    void processTopographyOverlay(
        const IntermediateData & data,
        TopographyProcessor & processor) override;
    void renderOverlay(
        sf::RenderWindow & window,
        TopographyProcessor & processor) override;
    void processOverlayEvent(
        const sf::Event & event,
        const sf::Vector2f & mouse,
        TopographyProcessor & processor) override;
    void saveOverlay(Save & save) const override;
    void loadOverlay(const Save & save) override;
};
