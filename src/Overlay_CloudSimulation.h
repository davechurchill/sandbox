#pragma once

#include "TopographyOverlay.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

class Overlay_CloudSimulation : public TopographyOverlay
{
    struct Cloud
    {
        cv::Point2f position;
        cv::Point2f direction = { 1.0f, 0.0f };
        float phase = 0.0f;
        float size = 1.0f;
        int avoidanceSide = 1;
    };

    cv::Mat                 m_topography;
    cv::Size                m_topographySize;
    TopographyProcessor *   m_processor = nullptr;
    std::vector<Cloud>      m_clouds;
    std::mt19937            m_random{ std::random_device{}() };

    float m_cloudHeight = 0.65f;
    float m_speedMultiplier = 1.0f;
    float m_cloudSize = 1.0f;
    int   m_cloudCount = 6;

    void resetClouds();
    float terrainHeight(const cv::Mat & terrain, const cv::Point2f & position) const;
    bool isOpenAir(const cv::Mat & terrain, const cv::Point2f & position) const;
    bool findSpawnPosition(const cv::Mat & terrain, cv::Point2f & position);
    float pathScore(
        const cv::Mat & terrain,
        const cv::Point2f & position,
        const cv::Point2f & direction,
        float distance) const;
    cv::Point2f chooseDirection(
        const cv::Mat & terrain,
        const Cloud & cloud,
        float lookAhead) const;
    void relocateAfterWrap(const cv::Mat & terrain, Cloud & cloud);
    void updateClouds(const cv::Mat & terrain, float deltaTime);
    void renderClouds(sf::RenderWindow & window);

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
