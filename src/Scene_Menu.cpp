#include "Scene_Menu.h"
#include "Scene_Main.h"
#include "GameEngine.h"
#include "Assets.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include "imgui.h"
#include "imgui-SFML.h"


Scene_Menu::Scene_Menu(GameEngine * game)
    : Scene(game)
{
    init();
}

void Scene_Menu::init()
{
    m_font = Assets::Instance().getFont("Tech");
    m_text.setFont(m_font);
    m_text.setPosition(10, 5);
    m_text.setCharacterSize(64);
    m_text.setString("AR SandBox\nPress any key to continue");
}

void Scene_Menu::onFrame()
{
    sUserInput();
    sRender(); 
    m_currentFrame++;
}

void Scene_Menu::sUserInput()
{
    sf::Event event;
    while (m_game->window().pollEvent(event))
    {
        ImGui::SFML::ProcessEvent(m_game->window(), event);

        // this event triggers when the window is closed
        if (event.type == sf::Event::Closed)
        {
            m_game->quit();
        }

        // this event is triggered when a key is pressed
        if (event.type == sf::Event::KeyPressed)
        {
            switch (event.key.code)
            {
            case sf::Keyboard::Escape: { exit(0); } break;
            default: m_game->changeScene<Scene_Main>("Main"); break;
            }
        }
    }
}

// renders the scene
void Scene_Menu::sRender()
{
    const sf::Color gridColor(64, 64, 64);

    m_game->window().clear();
    m_lineStrip.clear();
    m_quadArray.clear();

    m_game->window().draw(m_quadArray);
    m_game->window().draw(m_lineStrip);
    m_game->window().draw(m_text);
}
