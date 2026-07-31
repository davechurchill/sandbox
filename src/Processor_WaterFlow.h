#pragma once

#include "SandboxProjector.h"
#include "TopographyProcessor.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Processor_WaterFlow : public TopographyProcessor
{
    SandBoxProjector    m_projector;

    cv::Mat             m_water;
    cv::Mat             m_nextWater;
    cv::Mat             m_wetness;
    cv::Mat             m_encodedImage;
    cv::Mat             m_projectedImage;

    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite;
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
    int                 m_rainMode = 0;
    bool                m_hasFrame = false;
    bool                m_shaderLoaded = false;
    bool                m_rainBrushActive = false;
    bool                m_rainPulsePending = false;
    cv::Point2f         m_rainBrushPosition;

    void ensureSimulationSize(const cv::Size & size);
    void addRain(const cv::Mat & terrain, float amount);
    void simulate(const cv::Mat & terrain, float deltaTime);
    void buildImage(const cv::Mat & terrain);
    void resetWater();
    void reloadShader();
    bool mapMouseToTerrain(const sf::Vector2f & mouse, cv::Point2f & terrainPosition);

public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);

    void processTopography(const IntermediateData & data);
};
