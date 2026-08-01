#pragma once

#include "TopographyOverlay.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

class Overlay_SmokeFire : public TopographyOverlay
{
    struct Fire
    {
        cv::Point2f position;
        float age = 0.0f;
        float lifetime = 8.0f;
        float spreadTimer = 0.5f;
        float flameAccumulator = 0.0f;
        float smokeAccumulator = 0.0f;
    };

    struct FlameParticle
    {
        cv::Point2f position;
        cv::Point2f velocity;
        sf::Color color;
        float altitude = 0.0f;
        float riseSpeed = 0.1f;
        float age = 0.0f;
        float lifetime = 0.5f;
        float size = 3.0f;
        float phase = 0.0f;
    };

    struct Smoke
    {
        cv::Point2f position;
        cv::Point2f velocity;
        float altitude = 0.0f;
        float age = 0.0f;
        float lifetime = 4.0f;
        float size = 1.0f;
        float phase = 0.0f;
    };

    static constexpr size_t MaximumFires = 180;
    static constexpr size_t MaximumFlames = 1800;
    static constexpr size_t MaximumSmoke = 800;

    cv::Mat                 m_topography;
    cv::Size                m_topographySize;
    TopographyProcessor *   m_processor = nullptr;
    std::vector<Fire>       m_fires;
    std::vector<FlameParticle> m_flames;
    std::vector<Smoke>      m_smoke;
    std::mt19937            m_random{ std::random_device{}() };

    float m_fireSize = 16.0f;
    float m_fireLifetime = 9.0f;
    float m_spreadRate = 0.65f;
    float m_smokeAmount = 1.0f;
    float m_buoyancy = 1.0f;
    float m_windX = 0.15f;
    float m_windY = 0.0f;

    float sampleHeight(const cv::Point2f & position) const;
    bool isBurnable(const cv::Point2f & position) const;
    bool mapMouseToTerrain(
        const sf::Vector2f & mouse,
        TopographyProcessor & processor,
        cv::Point2f & terrainPosition) const;
    bool ignite(const cv::Point2f & position);
    void spawnFlame(const Fire & fire);
    void spawnSmoke(const Fire & fire);
    float fireIntensity(const Fire & fire) const;
    void updateSimulation(float deltaTime);
    void renderFlameParticles(sf::RenderWindow & window, TopographyProcessor & processor) const;
    void renderSmoke(sf::RenderWindow & window, TopographyProcessor & processor) const;
    void resetSimulation();

public:
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
    void saveOverlay(Settings & save) const override;
    void loadOverlay(const Settings & save) override;
};
