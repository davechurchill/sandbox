#include "Scene_Image.h"
#include "Scene_Menu.h"
#include "GameEngine.h"
#include "Assets.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include "imgui.h"
#include "imgui-SFML.h"


Scene_Image::Scene_Image(GameEngine * game)
    : Scene(game)
{
    init();
}

void Scene_Image::init()
{
    ImGui::GetStyle().ScaleAllSizes(2.0f);
    ImGui::GetIO().FontGlobalScale = 2.0f;
            
    m_font = Assets::Instance().getFont("Tech");
    m_text.setFont(m_font);
    m_text.setPosition(10, 5);
    m_text.setCharacterSize(10);

}

void Scene_Image::onFrame()
{
    sUserInput();
    sRender(); 
    renderUI();
    m_currentFrame++;
}

void Scene_Image::sUserInput()
{
    sf::Event event;
    while (m_game->window().pollEvent(event))
    {
        ImGui::SFML::ProcessEvent(m_game->window(), event);
        m_viewController.processEvent(m_game->window(), event);
        m_calibration.processEvent(event, m_mouseWorld);

        // this event triggers when the window is closed
        if (event.type == sf::Event::Closed)
        {
            m_game->quit();
        }

        // this event is triggered when a key is pressed
        if (!ImGui::GetIO().WantCaptureKeyboard && event.type == sf::Event::KeyPressed)
        {
            switch (event.key.code)
            {
                case sf::Keyboard::Escape:
                {
                    m_game->changeScene<Scene_Menu>("Menu");
                    break;
                }
            }
        }
        if (!ImGui::GetIO().WantCaptureMouse)
        {
            if (event.type == sf::Event::MouseButtonPressed)
            {
                // happens when the left mouse button is pressed
                if (event.mouseButton.button == sf::Mouse::Left) {}
                if (event.mouseButton.button == sf::Mouse::Right) {}
            }

            // happens when the mouse button is released
            if (event.type == sf::Event::MouseButtonReleased)
            {
                if (event.mouseButton.button == sf::Mouse::Left) {}
                if (event.mouseButton.button == sf::Mouse::Right) {}
            }

            // happens whenever the mouse is being moved
            if (event.type == sf::Event::MouseMoved)
            {
                m_mouseScreen = { event.mouseMove.x, event.mouseMove.y };
                m_mouseWorld = m_game->window().mapPixelToCoords(m_mouseScreen);
            }
        }
    }
}

// renders the scene
void Scene_Image::sRender()
{
    const sf::Color gridColor(64, 64, 64);

    m_game->window().clear();
    m_lineStrip.clear();
    m_quadArray.clear();

    m_game->window().draw(m_Sprite);
    
    m_game->window().draw(m_quadArray);
    m_game->window().draw(m_lineStrip);
    m_game->window().draw(m_text);
    m_calibration.render(m_game->window());
}

void Scene_Image::updateSprite()
{
    //m_calibration.transform(m_imageMatrix);
    sf::Image i;
    i.create(m_imageMatrix.cols, m_imageMatrix.rows, m_imageMatrix.ptr());
    m_sfTexture.loadFromImage(i);
    m_Sprite.setTexture(m_sfTexture, true);
}

void Scene_Image::renderUI()
{  
    ImGui::Begin("Options");

    if (ImGui::BeginTabBar("MyTabBar"))
    {
        if (ImGui::BeginTabItem("Picture"))
        {
            ImGui::InputTextWithHint("", "filename in /images", m_filename, 128);
            ImGui::SameLine();
            if (ImGui::Button("Load Image"))
            {
                m_imageMatrix = cv::imread(std::format("../images/{}", m_filename));
                cv::cvtColor(m_imageMatrix, m_imageMatrix, cv::COLOR_BGR2RGBA);
                updateSprite();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Calibration"))
        {
            m_calibration.imgui();
            if (ImGui::Button("Reload Image"))
            {
                m_imageMatrix = cv::imread(std::format("../images/{}", m_filename));
                cv::cvtColor(m_imageMatrix, m_imageMatrix, cv::COLOR_BGR2RGBA);
                updateSprite();
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}