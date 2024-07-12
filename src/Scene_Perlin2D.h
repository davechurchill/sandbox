#pragma once

#include "Vec2.hpp"
#include "Scene.h"
#include "ViewController.hpp"
#include "Grid.hpp"
#include "Perlin.hpp"

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

    int                 m_selectedShaderIndex = 0;
    
    int                 m_mcHeight = 30;

    Grid<float>         m_grid;

    sf::Vector2f        m_mouseScreen;
    sf::Vector2f        m_mouseWorld;
    Vec2                m_mouseGrid;
    
    ViewController      m_viewController;

    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite;

    sf::Shader          m_shader;
    
    void init();  

    void renderUI();
    void sUserInput();  
    void sRender();
    void calculateNoise();
    void imageFromGrid();
    
public:

    Scene_Perlin2D(GameEngine * game);

    void onFrame();
};
