#pragma once

#include "Scene.h"
#include "ViewController.hpp"
#include "Grid.hpp"
#include "Perlin.hpp"

#include <chrono>
#include <iostream>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <opencv2/opencv.hpp>   // Include OpenCV API

class Scene_Image : public Scene
{   
    sf::Font            m_font;             
    sf::Text            m_text;

    int                 m_colorAlpha = 255;
    float               m_colorPos[2] = { 0, 0 };
    float               m_colorScale = 1.0f;

    sf::Texture         m_sfColorTexture;
    sf::Sprite          m_colorSprite;

    sf::Vector2i        m_mouseScreen;
    sf::Vector2f        m_mouseWorld;
    
    ViewController      m_viewController;

    char                m_filename[128] = "";

    void init();  
    void renderUI();
    void sUserInput();  
    void sRender();
    
public:

    Scene_Image(GameEngine * game);

    void onFrame();
};
