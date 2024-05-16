#pragma once

#include "Vec2.hpp"
#include "Scene.h"
#include "WorldView.hpp"
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
    int                 m_waterLevel = 80;
    bool                m_drawContours = false;
    int                 m_contourSkip = 20;
    float               m_contourLevel = 0.5;
    int                 m_contourDiff = 3;
    Grid<char>          m_onContour;

    Grid<float>         m_grid;

    sf::Vector2f        m_mouseScreen;
    sf::Vector2f        m_mouseWorld;
    Vec2                m_mouseGrid;
    
    WorldView           m_view;
    
    void init();  

    void renderUI();
    void sUserInput();  
    void sRender();
    void calculateNoise();
    
public:

    Scene_Perlin2D(GameEngine * game);

    void onFrame();
};
