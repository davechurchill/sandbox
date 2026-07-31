#pragma once

#include "SandboxProjector.h"
#include "TopographyOverlay.h"
#include "TopographyProcessor.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

class Processor_Nature : public TopographyProcessor, public TopographyOverlay
{
    struct Sheep
    {
        cv::Point2f position;
        cv::Point2f direction = { 1.0f, 0.0f };
        cv::Point2f desiredDirection = { 1.0f, 0.0f };
        float directionTimer = 0.0f;
        float animationPhase = 0.0f;
        bool avoidingObstacle = false;
    };

    struct Wolf
    {
        cv::Point2f position;
        cv::Point2f direction = { 1.0f, 0.0f };
        cv::Point2f desiredDirection = { 1.0f, 0.0f };
        float animationPhase = 0.0f;
        float avoidanceTimer = 0.0f;
    };

    SandBoxProjector    m_projector;

    cv::Mat             m_topography;
    cv::Mat             m_projectedTopography;
    cv::Size            m_topographySize;

    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite;
    sf::Shader          m_shader;

    std::vector<Sheep>  m_sheep;
    Wolf                m_wolf;
    std::mt19937        m_random{ std::random_device{}() };

    float               m_sheepSpeed = 1.0f;
    float               m_sheepSize = 11.0f;
    float               m_waterLevel = 0.28f;
    int                 m_terrainType = 0;
    bool                m_defaultSheepCreated = false;
    bool                m_wolfCreated = false;
    bool                m_hasFrame = false;
    bool                m_shaderLoaded = false;
    TopographyProcessor * m_overlayProcessor = nullptr;

    void reloadShader();
    void resetAnimals();
    void updateSheep(const cv::Mat & terrain, float deltaTime);
    void updateWolf(const cv::Mat & terrain, float deltaTime);
    void randomizeDirection(Sheep & sheep);
    bool spawnRandomSheep(const cv::Mat & terrain);
    bool spawnRandomWolf(const cv::Mat & terrain);
    bool spawnSheep(const cv::Point2f & position);
    bool isValidTerrainPosition(const cv::Mat & terrain, const cv::Point2f & position) const;
    bool mapMouseToTerrain(const sf::Vector2f & mouse, cv::Point2f & terrainPosition);
    void renderSheep(sf::RenderWindow & window);
    void renderWolf(sf::RenderWindow & window);
    void drawSheep(
        sf::RenderWindow & window,
        const sf::Vector2f & position,
        const sf::Vector2f & direction,
        const Sheep & sheep) const;
    void drawWolf(
        sf::RenderWindow & window,
        const sf::Vector2f & position,
        const sf::Vector2f & direction) const;

public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);
    SandBoxProjector & projector() { return m_projector; }
    bool isTerrainWalkable(const cv::Mat & terrain, const cv::Point2f & position) const;

    void processTopography(const IntermediateData & data);

    void initOverlay();
    void imguiOverlay();
    void processTopographyOverlay(const IntermediateData & data, TopographyProcessor & processor);
    void renderOverlay(sf::RenderWindow & window, TopographyProcessor & processor);
    void processOverlayEvent(
        const sf::Event & event,
        const sf::Vector2f & mouse,
        TopographyProcessor & processor);
    void saveOverlay(Save & save) const;
    void loadOverlay(const Save & save);
};
