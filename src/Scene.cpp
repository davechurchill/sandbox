#include "Scene.h"
#include "GameEngine.h"

Scene::Scene(GameEngine * game)
    : m_game(game)
    , m_lineStrip(sf::PrimitiveType::LineStrip)
    , m_quadArray(sf::PrimitiveType::Triangles)
{ 
    
}
