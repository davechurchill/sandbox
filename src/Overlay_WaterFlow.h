#pragma once

#include "TopographyOverlay.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Overlay_WaterFlow : public TopographyOverlay
{
    cv::Mat             m_water;
    cv::Mat             m_nextWater;
    cv::Mat             m_wetness;
    cv::Mat             m_encodedImage;
    cv::Mat             m_projectedImage;

    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite{ m_texture };
    sf::Shader          m_shader;

    float               m_rainfall = 0.004f;
    float               m_flowSpeed = 8.0f;
    float               m_evaporation = 0.08f;
    float               m_waterDepthScale = 0.35f;
    float               m_trailPersistence = 2.5f;
    float               m_displayScale = 20.0f;
    float               m_waterOpacity = 0.9f;
    float               m_waterColor[3] = { 0.04f, 0.35f, 1.0f };
    float               m_rainRadius = 32.0f;
    int                 m_simulationSteps = 2;
    int                 m_rainMode = 1;
    bool                m_hasFrame = false;
    bool                m_shaderLoaded = false;
    bool                m_rainBrushActive = false;
    bool                m_rainPulsePending = false;
    cv::Point2f         m_rainBrushPosition;
    TopographyProcessor * m_overlayProcessor = nullptr;

    SandBoxProjector & activeProjector();
    void ensureSimulationSize(const cv::Size & size);
    void addRain(const cv::Mat & terrain, float amount);
    void simulate(const cv::Mat & terrain, float deltaTime);
    void buildImage(const cv::Mat & terrain);
    void resetWater();
    void reloadShader();
    void imguiControls();
    void renderWater(sf::RenderWindow & window);
    void handleInput(const sf::Event & event, const sf::Vector2f & mouse);
    bool mapMouseToTerrain(const sf::Vector2f & mouse, cv::Point2f & terrainPosition);

public:
    bool usesCanvasInput() const override { return m_rainMode == 1; }
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
