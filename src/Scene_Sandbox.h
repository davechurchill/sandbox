#pragma once

#include "Scene.h"
#include "ViewController.hpp"
#include "Grid.hpp"
#include "Calibration.h"
#include "ContourLines.hpp"

#include <chrono>
#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API

enum class alignment
{
    depth,
    color,
    nothing
};

enum class MouseSelections
{
    None,
    MaxDistance,
    MinDistance
};

class Scene_Sandbox : public Scene
{   
    bool                m_cameraConnected = false;

    sf::Font            m_font;             
    sf::Text            m_text;

    bool                m_drawDepth = true;
    bool                m_drawColor = false; 
    bool                m_drawUI = true;

    int                 m_depthAlpha = 255;
    float               m_depthPos[2] = { 0, 0 };
    float               m_depthScaleX = 1.0f;
    float               m_depthScaleY = 1.0f;
    int                 m_colorAlpha = 255;
    float               m_colorPos[2] = { 0, 0 };
    float               m_colorScale = 1.0f;

    alignment           m_alignment = alignment::nothing;
    MouseSelections     m_mouseSelection = MouseSelections::None;

    rs2::colorizer      m_colorMap;
    rs2::pipeline       m_pipe;
    rs2::align          m_alignment_depth = rs2::align(RS2_STREAM_DEPTH);
    rs2::align          m_alignment_color = rs2::align(RS2_STREAM_COLOR);

    cv::Mat             m_cvRawDepthImage;
    cv::Mat             m_cvColorImage;

    sf::Image           m_sfDepthImage;
    sf::Image           m_sfColorImage;
    sf::Texture         m_sfDepthTexture;
    sf::Texture         m_sfColorTexture;
    sf::Sprite          m_depthSprite;
    sf::Sprite          m_colorSprite;

    sf::Image           m_transformedImage;
    sf::Texture         m_transformedTexture;
    sf::Sprite          m_transformedSprite;

    sf::Vector2i        m_mouseScreen;
    sf::Vector2f        m_mouseWorld;
    
    ViewController      m_viewController;

    Calibration         m_calibration;

    Grid<float>         m_depthGrid;
    float               m_mouseDepth;

    int                 m_decimation = 1;
    rs2::decimation_filter m_decimationFilter;

    float               m_maxDistance = 5.0;
    float               m_minDistance = 0.0;
    rs2::threshold_filter m_thresholdFilter;

    int                 m_spatialMagnitude = 2;
    float               m_smoothAlpha = 0.5;
    int                 m_smoothDelta = 20;
    int                 m_spatialHoleFill = 0;
    rs2::spatial_filter m_spatialFilter;

   
    float m_smoothAlphaTemporal = 0.4;
    rs2::temporal_filter m_temporalFilter;

    int                 m_holeFill = 1;
    rs2::hole_filling_filter m_holeFilter;

    bool                m_drawContours = false;
    ContourLines        m_contour;
    sf::Sprite          m_contourSprite;

    int                 m_numberOfContourLines = 5;
    
    void captureImage();

    void init();  
    void renderUI();
    void sUserInput();  
    void sRender();
    void attemptCameraConnection();
    void saveConfig();
    void loadConfig();

    sf::Color colorize(float height);
    
public:

    Scene_Sandbox(GameEngine * game);

    void onFrame();
};
