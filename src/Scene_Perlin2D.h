#pragma once

#include "Vec2.hpp"
#include "Scene.h"
#include "ViewController.hpp"
#include "Grid.hpp"
#include "Perlin.hpp"
#include "ContourLines.hpp"
#include "Colorizer.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <chrono>
#include <iostream>

class Scene_Perlin2D : public Scene
{   
    sf::Font            m_font;             
    sf::Text            m_text;

    float               m_gridSize = 32;
    bool                m_drawGrid = false;
    bool                m_drawGrey = false;


    Perlin2DNew         m_perlin;
    int                 m_octaves = 5;
    int                 m_seed = 0;
    int                 m_seedSize = 9;
    float               m_persistance = 0.5f;
    bool                m_drawContours = false;
    int                 m_contourSkip = 20;
    float               m_contourLevel = 0.5;
    int                 m_numberOfContourLines = 5;
    
    int                 m_mcHeight = 30;

    ContourLines        m_contour;

    Grid<float>         m_grid;

    sf::Vector2f        m_mouseScreen;
    sf::Vector2f        m_mouseWorld;
    Vec2                m_mouseGrid;
    
    ViewController      m_viewController;

    sf::Sprite          m_contourSprite;

    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite;

    Colorizer           m_colorizer;
    
    void init();  

    void renderUI();
    void sUserInput();  
    void sRender();
    void calculateNoise();
    
public:

    Scene_Perlin2D(GameEngine * game);

    void onFrame();
};
