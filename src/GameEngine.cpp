#include "GameEngine.h"
#include "Assets.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include <iostream>

GameEngine::GameEngine()
{
    init();
}

void GameEngine::init()
{
    m_window.create(sf::VideoMode(1600, 900), "Grid View");
    //m_window.setFramerateLimit(60);

    ImGui::SFML::Init(m_window);
    m_originalStyle = ImGui::GetStyle();

    Assets::Instance().addFont("Tech", "fonts/tech.ttf");
}

bool GameEngine::isRunning()
{ 
    return m_running && m_window.isOpen();
}

sf::RenderWindow & GameEngine::window()
{
    return m_window;
}

std::shared_ptr<Scene> GameEngine::currentScene()
{
    return m_sceneMap.at(m_currentScene);
}

void GameEngine::update()
{
    if (!isRunning()) { return; }

    if (m_sceneMap.empty()) { return; }

    ImGui::SFML::Update(m_window, m_deltaClock.restart());

    currentScene()->onFrame();

    ImGui::SFML::Render(m_window);

    m_window.display();
}

void GameEngine::run()
{
    while (isRunning())
    {
        update();
    }
}

void GameEngine::quit()
{
    m_running = false;
}

unsigned int GameEngine::width() const
{
    return m_window.getSize().x;
}

unsigned int GameEngine::height() const
{
    return m_window.getSize().x;
}