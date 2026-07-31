#pragma once

#include "SandboxProjector.h"
#include "TopographyProcessor.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

class Processor_Balls : public TopographyProcessor
{
    struct Ball
    {
        cv::Point2f position;
        cv::Point2f velocity;
        sf::Color color = sf::Color(190, 58, 42);
        float rotation = 0.0f;
    };

    SandBoxProjector    m_projector;

    cv::Mat             m_topography;
    cv::Mat             m_projectedTopography;
    cv::Size            m_topographySize;

    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite;
    sf::Shader          m_shader;

    std::vector<Ball>   m_balls;
    std::mt19937        m_random{ std::random_device{}() };

    float               m_gravity = 1800.0f;
    float               m_ballSpeedMultiplier = 1.0f;
    float               m_rollingResistance = 0.8f;
    float               m_ballSize = 18.0f;
    float               m_ballRestitution = 0.65f;
    bool                m_defaultBallCreated = false;
    bool                m_randomResetPending = false;
    bool                m_hasFrame = false;
    bool                m_shaderLoaded = false;

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
    void renderBalls(sf::RenderWindow & window);
    void drawBall(
        sf::RenderWindow & window,
        const sf::Vector2f & position,
        const sf::Vector2f & direction,
        const sf::Color & color,
        float rotation) const;

public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);

    void processTopography(const IntermediateData & data);
};
