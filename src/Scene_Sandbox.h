#pragma once

#include "Scene.h"
#include "ViewController.hpp"
#include "Grid.hpp"
#include "Calibration.h"
#include "ContourLines.hpp"
#include "Colorizer.hpp"
#include "CameraFilters.hpp"

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
    bool                m_cameraConnected = false;

    sf::Font            m_font;             
    sf::Text            m_text;

    bool                m_drawDepth = true;
    bool                m_drawColor = false; 
    bool                m_drawUI = true;

    alignment           m_alignment = alignment::nothing;

    rs2::pipeline       m_pipe;
    rs2::align          m_alignment_depth = rs2::align(RS2_STREAM_DEPTH);
    rs2::align          m_alignment_color = rs2::align(RS2_STREAM_COLOR);

    cv::Mat             m_cvRawDepthImage;
    cv::Mat             m_cvColorImage;

    sf::Image           m_sfDepthImage;
    sf::Image           m_transformedImage;
    sf::Image           m_sfColorImage;
    sf::Texture         m_sfDepthTexture;
    sf::Texture         m_transformedTexture;
    sf::Texture         m_sfColorTexture;
    sf::Sprite          m_depthSprite;
    sf::Sprite          m_transformedSprite;
    sf::Sprite          m_colorSprite;

    sf::Vector2i        m_mouseScreen;
    sf::Vector2f        m_mouseWorld;
    
    ViewController      m_viewController;

    Calibration         m_calibration;

    Colorizer           m_colorizer;

    Grid<float>         m_depthGrid;
    Grid<float>         m_depthWarpedGrid;

    float               m_maxDistance = 1.13f;
    float               m_minDistance = 0.90f;

    CameraFilters       m_filters;

    bool                m_drawContours = false;
    ContourLines        m_contour;
    sf::Sprite          m_contourSprite;

    int                 m_numberOfContourLines = 5;
    
    void captureImage();

    void init();  
    void renderUI();
    void sUserInput();  
    void sProcessEvent(const sf::Event & event);
    void sRender();
    void attemptCameraConnection();
    void saveConfig();
    void loadConfig();
    
public:

    Scene_Sandbox(GameEngine * game);

    void onFrame();
    void endScene();
};
