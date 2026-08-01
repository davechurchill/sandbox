#pragma once

#include "TopographyVisualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

class Visualizer_FishPond : public TopographyVisualizer
{
    static constexpr int MaximumFishCount = 1000;

    struct Fish
    {
        cv::Point2f position;
        cv::Point2f velocity = { 1.0f, 0.0f };
        cv::Point2f wanderDirection = { 1.0f, 0.0f };
        sf::Color color = sf::Color(245, 145, 35);
        int colorType = 0;
        float phase = 0.0f;
        float wanderTimer = 0.0f;
        float swimDepth = 0.07f;
    };

    cv::Mat             m_topography;
    cv::Mat             m_projectedTopography;
    cv::Size            m_topographySize;
    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite{ m_texture };
    sf::Shader          m_shader;

    std::vector<Fish>   m_fish;
    std::mt19937        m_random{ std::random_device{}() };

    int   m_targetFishCount = 100;
    float m_speedMultiplier = 1.0f;
    float m_fishSize = 12.0f;
    float m_schoolRadius = 48.0f;
    float m_schoolStrength = 1.0f;
    float m_separationStrength = 1.35f;
    float m_minimumDepth = 0.07f;
    float m_maximumFishDepth = 0.48f;
    bool  m_resetPending = true;
    bool  m_hasFrame = false;
    bool  m_shaderLoaded = false;

    void reloadShader();
    float sampleHeight(const cv::Point2f & position) const;
    bool isSwimmable(const cv::Point2f & position) const;
    bool isSwimmable(const cv::Point2f & position, float swimDepth) const;
    bool addFish(const cv::Point2f & position);
    bool spawnRandomFish();
    void resetFish();
    void randomizeWander(Fish & fish);
    cv::Point2f shorelineSteering(const Fish & fish, const cv::Point2f & forward) const;
    void updateFish(float deltaTime);
    bool mapMouseToTerrain(const sf::Vector2f & mouse, cv::Point2f & terrainPosition);
    void renderFish(sf::RenderWindow & window);
    void drawFish(
        sf::RenderWindow & window,
        const sf::Vector2f & position,
        const sf::Vector2f & direction,
        const Fish & fish) const;

public:
    static constexpr std::string_view Name = "Fish Pond";
    Visualizer_FishPond() : TopographyVisualizer(Name) {}

    void init() override;
    void imgui() override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
    bool isTerrainWalkable(const cv::Mat &, const cv::Point2f &) const override { return false; }
    bool definesTerrainWalkability() const override { return true; }

    void process(const TerrainFrame & data) override;
};
