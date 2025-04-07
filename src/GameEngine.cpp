#include "GameEngine.h"
#include "Profiler.hpp"

#include "imgui.h"
#include "imgui-SFML.h"

#include <iostream>
#include <Scene_Main.h>

GameEngine::GameEngine()
{
    init();
}

void GameEngine::init()
{
    m_window.create(sf::VideoMode(1600, 900), "Sandbox");
    //m_window.setFramerateLimit(60);

    ImGui::SFML::Init(m_window);
    m_originalStyle = ImGui::GetStyle();
}

bool GameEngine::isRunning()
{ 
    return m_running && m_window.isOpen();
}

sf::RenderWindow & GameEngine::window()
{
    return m_window;
}

sf::RenderWindow & GameEngine::displayWindow()
{
    return m_displayWindow;
}

std::shared_ptr<Scene> GameEngine::currentScene()
{
    return m_sceneMap.at(m_currentScene);
}

void GameEngine::update()
{
    if (!isRunning()) { return; }

    if (m_sceneMap.empty()) { return; }

    sf::Time dt = m_deltaClock.restart();
    m_framerate = m_framerate * 0.75f + 0.25f / dt.asSeconds();

    {
        PROFILE_SCOPE("ImGui::Update");
        ImGui::SFML::Update(m_window, dt);
    }

    float deltaTime = dt.asMicroseconds() / 1000000.f;
    currentScene()->onFrame(deltaTime);

    ImGui::SFML::Render(m_window);

    {
        PROFILE_SCOPE("window.display()");
        m_window.display();

        if (m_displayWindow.isOpen())
        {
            m_displayWindow.display();
        }
    }
}

void GameEngine::run()
{
    std::dynamic_pointer_cast<Scene_Main>(currentScene())->load();
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

float GameEngine::framerate() const
{
    return m_framerate;
}
