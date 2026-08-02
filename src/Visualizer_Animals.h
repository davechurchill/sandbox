#pragma once

#include "Visualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

class Visualizer_Animals : public Visualizer
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

    cv::Mat             m_topography;
    cv::Size            m_topographySize;

    std::vector<Sheep>  m_sheep;
    Wolf                m_wolf;
    std::mt19937        m_random{ std::random_device{}() };

    float               m_sheepSpeed = 1.0f;
    float               m_sheepSize = 11.0f;
    bool                m_defaultSheepCreated = false;
    bool                m_wolfCreated = false;
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
    void drawSheep(sf::RenderWindow & window, const sf::Vector2f & position, const sf::Vector2f & direction, const Sheep & sheep) const;
    void drawWolf(sf::RenderWindow & window, const sf::Vector2f & position, const sf::Vector2f & direction) const;

public:
    static constexpr std::string_view Name = "Animals";
    explicit Visualizer_Animals(SandboxProjector & projector) : Visualizer(Name, projector) {}

    bool usesCanvasInput() const override { return true; }
    void init() override;
    void imgui() override;
    void process(const TerrainFrame & data) override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
