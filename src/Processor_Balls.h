#pragma once

#include "SandboxProjector.h"
#include "TopographyOverlay.h"
#include "TopographyProcessor.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

class Processor_Balls : public TopographyProcessor, public TopographyOverlay
{
    struct Ball
    {
        cv::Point2f position;
        cv::Point2f velocity;
        sf::Color color = sf::Color(190, 58, 42);
        float rotation = 0.0f;
        float trailTimer = 0.0f;
    };

    struct BallTrail
    {
        cv::Point2f position;
        sf::Color color;
        float remaining = 0.0f;
        float lifetime = 0.0f;
        bool lava = false;
    };

    SandBoxProjector    m_projector;

    cv::Mat             m_topography;
    cv::Mat             m_projectedTopography;
    cv::Size            m_topographySize;

    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite{ m_texture };
    sf::Shader          m_shader;
    sf::Shader          m_ballShader;

    std::vector<Ball>   m_balls;
    std::vector<BallTrail> m_trails;
    std::mt19937        m_random{ std::random_device{}() };

    float               m_gravity = 1800.0f;
    float               m_ballSpeedMultiplier = 1.0f;
    float               m_rollingResistance = 0.4f;
    float               m_ballSize = 18.0f;
    float               m_ballRestitution = 0.65f;
    float               m_trailLength = 0.0f;
    bool                m_lavaAppearance = false;
    bool                m_defaultBallCreated = false;
    bool                m_randomResetPending = false;
    bool                m_hasFrame = false;
    bool                m_shaderLoaded = false;
    bool                m_ballShaderLoaded = false;
    TopographyProcessor * m_overlayProcessor = nullptr;

    SandBoxProjector & activeProjector();
    void reloadShader();
    void resetBalls();
    sf::Color randomBallColor();
    bool addBall(const cv::Point2f & position);
    bool spawnBallNearMiddle(const cv::Mat & terrain);
    bool spawnRandomBall(const cv::Mat & terrain);
    bool findRandomTerrainPosition(const cv::Mat & terrain, cv::Point2f & position);
    bool isValidTerrainPosition(const cv::Mat & terrain, const cv::Point2f & position) const;
    bool sampleTerrainHeight(const cv::Mat & terrain, const cv::Point2f & position, float & height) const;
    bool mapMouseToTerrain(const sf::Vector2f & mouse, cv::Point2f & terrainPosition);
    float getTerrainBallRadius(const cv::Mat & terrain) const;
    void resolveBallCollisions(const cv::Mat & terrain);
    void updateBalls(const cv::Mat & terrain, float deltaTime);
    void renderBallTrails(sf::RenderWindow & window);
    void renderBalls(sf::RenderWindow & window);
    void drawBall(
        sf::RenderWindow & window,
        const sf::Vector2f & position,
        const sf::Vector2f & direction,
        const sf::Color & color,
        float rotation,
        float visualScale,
        float movementAmount);

public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);
    SandBoxProjector & projector() { return m_projector; }

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
