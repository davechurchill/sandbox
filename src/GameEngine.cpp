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
    m_window.create(sf::VideoMode(1600, 900), "Sandbox");
    //m_window.setFramerateLimit(60);

    ImGui::SFML::Init(m_window);
    m_originalStyle = ImGui::GetStyle();

    Assets::Instance().addFont("Tech", "fonts/tech.ttf");
}

bool GameEngine::isRunning()
{ 
    return m_running && m_window.isOpen();
}

void GameEngine::toggleFullscreen()
{
    if (m_isFullscreen)
    {
        m_window.create(sf::VideoMode(1600, 900), "Sandbox");
        m_isFullscreen = false;
    }
    else
    {
        m_window.create(sf::VideoMode(), "Sandbox", sf::Style::Fullscreen);
        m_isFullscreen = true;
    }
}

sf::RenderWindow & GameEngine::window()
{
    return m_window;
}

sf::RenderWindow & GameEngine::displayWindow()
{
    return m_displayWindow;
}

mc::MinecraftInterface & GameEngine::minecraft()
{
    return m_mcInterface;
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
    ImGui::SFML::Update(m_window, dt);

    currentScene()->onFrame();

    ImGui::SFML::Render(m_window);

    m_window.display();

    if (m_displayWindow.isOpen())
    {
        m_displayWindow.display();
    }
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

float GameEngine::framerate() const
{
    return m_framerate;
}
