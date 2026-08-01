#pragma once

#include "SandboxProjector.h"
#include "TopographyProcessor.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

class Processor_ForestFire final : public TopographyProcessor
{
    struct FireParticle
    {
        cv::Point2f position;
        cv::Point2f velocity;
        sf::Color color;
        float age = 0.0f;
        float lifetime = 0.6f;
        float size = 3.0f;
        float riseDistance = 8.0f;
        float phase = 0.0f;
    };

    SandBoxProjector m_projector;

    cv::Mat m_topography;
    cv::Mat m_burnableMask;
    cv::Mat m_initialFuel;
    cv::Mat m_fuel;
    cv::Mat m_fire;
    cv::Mat m_projectedState;
    cv::Size m_topographySize;

    sf::Image m_image;
    sf::Texture m_texture;
    sf::Sprite m_sprite{ m_texture };
    sf::Shader m_shader;

    std::vector<cv::Point2f> m_burningPositions;
    std::vector<FireParticle> m_fireParticles;

    std::mt19937 m_random{ std::random_device{}() };

    float m_waterLevel = 0.28f;
    float m_rockLevel = 0.72f;
    float m_rockSlope = 0.70f;
    float m_spreadRate = 0.75f;
    float m_burnRate = 0.16f;
    float m_windX = 0.45f;
    float m_windY = 0.0f;
    float m_ignitionRadius = 8.0f;
    float m_treeBrushSize = 30.0f;
    float m_treeBrushBlur = 12.0f;
    float m_treePaintAmount = 0.08f;
    float m_simulationAccumulator = 0.0f;
    float m_particleSpawnAccumulator = 0.0f;

    int m_burningCells = 0;
    int m_treeCells = 0;
    float m_fuelRemaining = 1.0f;

    bool m_paused = false;
    bool m_resetRequested = true;
    bool m_hasFrame = false;
    bool m_shaderLoaded = false;
    bool m_hasLastTreePaintPosition = false;
    cv::Point2f m_lastTreePaintPosition;

    void reloadShader();
    void initializeForest(const cv::Mat & terrain);
    void updateBurnableMask(const cv::Mat & terrain);
    void simulateStep(const cv::Mat & terrain, float deltaTime);
    void spawnFireParticle();
    void updateFireParticles(float deltaTime);
    void renderFireParticles(sf::RenderWindow & window);
    void updateStatistics();
    void updateTexture(const cv::Mat & terrain);
    bool mapMouseToTerrain(const sf::Vector2f & mouse, cv::Point2f & terrainPosition);
    void applyTreeBrush(const cv::Point2f & position, float direction);
    void paintTreeLine(
        const cv::Point2f & from,
        const cv::Point2f & to,
        float direction);
    void ignite(const cv::Point2f & position, float radius);
    bool igniteRandomFire();
    void extinguish();

public:
    void init() override;
    void imgui() override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Save & save) const override;
    void load(const Save & save) override;
    SandBoxProjector & projector() override { return m_projector; }
    void onSourceChanged() override;
    bool usesCanvasInput() const override { return true; }
    void processTopography(const IntermediateData & data) override;
};
