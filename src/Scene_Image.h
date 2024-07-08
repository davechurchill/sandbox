#pragma once

#include "Scene.h"
#include "ViewController.hpp"
#include "Grid.hpp"
#include "Perlin.hpp"
#include "Calibration.h"

#include <chrono>
#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <opencv2/opencv.hpp>   // Include OpenCV API

class Scene_Image : public Scene
{   
    sf::Font            m_font;             
    sf::Text            m_text;

    cv::Mat             m_imageMatrix;
    sf::Texture         m_sfTexture;
    sf::Sprite          m_Sprite;

    sf::Vector2i        m_mouseScreen;
    sf::Vector2f        m_mouseWorld;
    
    ViewController      m_viewController;
    Calibration         m_calibration;

    sf::Shader          m_shader;
    sf::Clock           m_imageClock;

    char                m_filename[128] = "test.png";

    void init();  
    void renderUI();
    void sUserInput();  
    void sRender();
    void updateSprite();
    
public:

    Scene_Image(GameEngine * game);

    void onFrame();
};
