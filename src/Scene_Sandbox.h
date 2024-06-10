#pragma once

#include "Scene.h"
#include "ViewController.hpp"
#include "Grid.hpp"
#include "Calibration.h"

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

class Scene_Sandbox : public Scene
{   
    sf::Font            m_font;             
    sf::Text            m_text;

    bool                m_drawDepth = true;
    bool                m_drawColor = false; 

    int                 m_depthAlpha = 255;
    float               m_depthPos[2] = { 0, 0 };
    float               m_depthScale = 1.0f;
    int                 m_colorAlpha = 255;
    float               m_colorPos[2] = { 0, 0 };
    float               m_colorScale = 1.0f;

    alignment           m_alignment = alignment::nothing;

    rs2::colorizer      m_colorMap;
    rs2::pipeline       m_pipe;
    rs2::align          m_alignment_depth = rs2::align(RS2_STREAM_DEPTH);
    rs2::align          m_alignment_color = rs2::align(RS2_STREAM_COLOR);

    cv::Mat             m_cvDepthImage;
    cv::Mat             m_cvColorImage;

    sf::Image           m_sfDepthImage;
    sf::Image           m_sfColorImage;
    sf::Texture         m_sfDepthTexture;
    sf::Texture         m_sfColorTexture;
    sf::Sprite          m_depthSprite;
    sf::Sprite          m_colorSprite;

    sf::Vector2i        m_mouseScreen;
    sf::Vector2f        m_mouseWorld;
    
    ViewController      m_viewController;

    Calibration         m_calibration;

    Grid<float>         m_depthGrid;

    int                 m_decimation = 1;
    rs2::decimation_filter m_decimationFilter;

    float               m_maxDistance = 16.0;
    float               m_minDistance = 0.0;
    rs2::threshold_filter m_thresholdFilter;
    
    void captureImage();

    void init();  
    void renderUI();
    void sUserInput();  
    void sRender();
    bool isCameraConnected();
    
public:

    Scene_Sandbox(GameEngine * game);

    void onFrame();
};
