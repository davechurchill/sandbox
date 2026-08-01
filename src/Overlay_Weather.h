#pragma once

#include "TopographyOverlay.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

class Overlay_Weather : public TopographyOverlay
{
    enum class Mode
    {
        Rain,
        Snow,
        Fog,
        Clouds
    };

    struct Particle
    {
        cv::Point2f position;
        float altitude = 1.0f;
        float phase = 0.0f;
        float size = 1.0f;
        float opacity = 1.0f;
    };

    cv::Mat                 m_topography;
    cv::Size                m_topographySize;
    TopographyProcessor *   m_processor = nullptr;
    std::vector<Particle>   m_particles;
    std::mt19937            m_random{ std::random_device{}() };

    int   m_mode = (int)Mode::Rain;
    float m_intensity = 0.55f;
    float m_windX = 0.20f;
    float m_windY = 0.02f;
    float m_elementSize = 1.0f;
    float m_fallSpeed = 1.0f;

    void resetParticles();
    void initializeParticle(Particle & particle, const cv::Mat & terrain, bool randomAltitude);
    float sampleHeight(const cv::Mat & terrain, const cv::Point2f & position) const;
    void wrapPosition(cv::Point2f & position, const cv::Size & size) const;
    int targetParticleCount() const;
    void updateParticles(const cv::Mat & terrain, float deltaTime);
    void renderPrecipitation(sf::RenderWindow & window, bool snow);
    void renderAtmosphere(sf::RenderWindow & window, bool clouds);

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
