#pragma once

#include "Vec2.hpp"
#include "Scene.h"

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <chrono>
#include <iostream>

class Scene_Menu : public Scene
{   
    sf::Font            m_font;             
    sf::Text            m_text;

    void init();  

    void sUserInput();  
    void sRender();
    
public:

    Scene_Menu(GameEngine * game);

    void onFrame();
};
