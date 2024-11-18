#pragma once

#include <memory>
#include <string>
#include <SFML/Graphics.hpp>

class GameEngine;

class Scene
{

protected: 
    
    GameEngine *    m_game = nullptr;
    bool            m_paused = false;
    size_t          m_currentFrame = 0;
    sf::VertexArray m_lineStrip;
    sf::VertexArray m_quadArray;
    sf::VertexArray m_triangleStrip;

    Scene(GameEngine * game);

public:

    virtual void onFrame(float deltaTime) = 0;

};