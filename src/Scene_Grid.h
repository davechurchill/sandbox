#pragma once

#include "Scene.h"
#include "WorldView.hpp"
#include "Grid.hpp"
#include "algorithms/Environment.h"
#include "algorithms/GridConnectivity.hpp"
#include "algorithms/GridLegalActions.hpp"
#include "algorithms/GridSearch.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <chrono>
#include <iostream>

class Scene_Grid : public Scene
{   
    sf::Font            m_font;             
    sf::Text            m_text;

    Environment         m_env;
    GridConnectivity    m_connect;
    GridLegalActions    m_legalActions;
    GridSearch          m_search;

    sf::Uint8*          m_gridPixels = nullptr;
    sf::Image           m_gridImage;
    sf::Texture         m_gridTexture;
    sf::Sprite          m_gridSprite;

    float               m_gridSize = 32;
    bool                m_drawGrid = false;
    bool                m_mouseButton[3] = { false, false, false };

    sf::Color           m_colors[255];

    State               m_mouseCell = { -1, -1 };
    State               m_startCell = { -1, -1 };
    State               m_goalCell = { -1, -1 };

    Vec2                m_drag = { -1, -1 };
    Vec2                m_mouseScreen;
    Vec2                m_mouseWorld;

    long long           m_lastSearch = 0;

    std::vector<std::string> m_mapFiles;
    
    WorldView           m_view;
    
    void init();  
    void createGridTexture();
    void addMapFiles(const std::string& dir);

    void renderUI();
    void sUserInput();  
    void sRender();

    void loadMap(const std::string& path);
    
public:

    Scene_Grid(GameEngine * game);

    void onFrame();
};
