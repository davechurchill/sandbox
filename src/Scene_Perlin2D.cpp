#include "Scene_Perlin2D.h"
#include "GameEngine.h"
#include "WorldView.hpp"
#include "Assets.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include "imgui.h"
#include "imgui-SFML.h"


Scene_Perlin2D::Scene_Perlin2D(GameEngine * game)
    : Scene(game)
{
    init();
}

void Scene_Perlin2D::init()
{
    ImGui::GetStyle().ScaleAllSizes(2.0f);
    ImGui::GetIO().FontGlobalScale = 2.0f;

    m_view.setWindowSize(Vec2(m_game->window().getSize().x, m_game->window().getSize().y));

    m_view.setView(m_game->window().getView());
    m_view.zoomTo(16, { 0, 0 });
    m_view.move({ -m_gridSize*3, -m_gridSize*3 });
        
    m_font = Assets::Instance().getFont("Tech");
    m_text.setFont(m_font);
    m_text.setPosition(10, 5);
    m_text.setCharacterSize(10);

    calculateNoise();

}

void Scene_Perlin2D::calculateNoise()
{
    m_perlin = Perlin2DNew((int)(1 << m_seedSize), (int)(1 << m_seedSize), m_seed);
    m_grid = m_perlin.GeneratePerlinNoise(m_octaves, m_persistance);
}

void Scene_Perlin2D::onFrame()
{
    m_view.update();
    sUserInput();
    sRender(); 
    renderUI();
    m_currentFrame++;
}

void Scene_Perlin2D::sUserInput()
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
                case sf::Keyboard::Escape:
                {
                    m_game->changeScene("MENU", nullptr, true);
                    break;
                }
                case sf::Keyboard::R: { m_seed += 1; calculateNoise(); break; }
                case sf::Keyboard::W: { m_octaves = std::min(m_octaves + 1, m_seedSize); calculateNoise();  break; }
                case sf::Keyboard::S: { m_octaves--; calculateNoise();  break; }
                case sf::Keyboard::A: { m_persistance -= 0.1f; if (m_persistance < 0.1f) { m_persistance = 0.1f; } calculateNoise();  break; }
                case sf::Keyboard::D: { m_persistance += 0.1f; calculateNoise();  break; }
                case sf::Keyboard::G: m_drawGrey = !m_drawGrey; break;
                case sf::Keyboard::Num2: { m_waterLevel++; break; }
                case sf::Keyboard::Num1: { m_waterLevel--; break; }
            }
        }

        if (event.type == sf::Event::KeyReleased)
        {
            switch (event.key.code)
            {
            case sf::Keyboard::W: break;
            case sf::Keyboard::A: break;
            case sf::Keyboard::S: break;
            case sf::Keyboard::D: break;
            }
        }

        if (event.type == sf::Event::MouseButtonPressed)
        {
            // happens when the left mouse button is pressed
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                m_view.stopScroll();
            }

            // happens when the right mouse button is pressed
            if (event.mouseButton.button == sf::Mouse::Right)
            {
                m_drag = { event.mouseButton.x, event.mouseButton.y };
                m_view.stopScroll();
            }
        }

        // happens when the mouse button is released
        if (event.type == sf::Event::MouseButtonReleased)
        {
            // let go of the currently selected rectangle
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                
            }

            // let go of the currently selected rectangle
            if (event.mouseButton.button == sf::Mouse::Right)
            {
                m_drag = { -1, -1 };
            }
        }

        if (event.type == sf::Event::MouseWheelMoved)
        {
            double zoom = 1.0 - (0.2 * event.mouseWheel.delta);
            m_view.zoomTo(zoom, Vec2(event.mouseWheel.x, event.mouseWheel.y));
        }

        // happens whenever the mouse is being moved
        if (event.type == sf::Event::MouseMoved)
        {
            m_mouseScreen = { event.mouseMove.x, event.mouseMove.y };

            // record the current mouse position in universe coordinates
            m_mouseWorld = m_view.windowToWorld(m_mouseScreen);

            // record the grid cell that the mouse position is currently over
            m_mouseGrid = { floor(m_mouseWorld.x / m_gridSize) * m_gridSize, floor(m_mouseWorld.y / m_gridSize) * m_gridSize };

            if (m_drag.x != -1)
            {
                auto prev = m_view.windowToWorld(m_drag);
                auto curr = m_view.windowToWorld({ event.mouseMove.x, event.mouseMove.y });
                auto scroll = prev - curr;
                m_view.scroll(prev - curr);
                m_drag = { event.mouseMove.x, event.mouseMove.y };
            }
        }
    }
}


// renders the scene
void Scene_Perlin2D::sRender()
{
    const sf::Color gridColor(64, 64, 64);

    m_game->window().clear();
    m_lineStrip.clear();
    m_quadArray.clear();
    float gs = (float)m_gridSize;
    m_game->window().setView(m_view.getSFMLView());

    // draw grid cells with the associated colors
    for (size_t x = 0; x < m_grid.width(); x++)
    {
        for (size_t y = 0; y < m_grid.height(); y++)
        {
            if (m_drawGrey)
            {
                // get the value from the grid
                int val = (int)(m_grid.get(x, y) * 255);
                
                // create a greyscale color to display
                sf::Color c(val, val, val);

                // draw the tile as a greyscale color
                drawRect<float>(x * m_gridSize, y * m_gridSize, m_gridSize, m_gridSize, c);
            }
            else
            {
                // get the value from the grid
                int val = (int)(m_grid.get(x, y) * 255);

                // the color of the tile
                sf::Color c;

                // if the value is less than some intuitive value
                if (val < m_waterLevel)
                { 
                    // color the tile like water from blue->black
                    c = sf::Color(0, 0, 255 - m_waterLevel + val);
                }
                // if the value is above that, it's not water
                else 
                { 
                    // color tile like grass getting brighter
                    c = sf::Color(0, val, 0);
                }

                drawRect<float>(x * m_gridSize, y * m_gridSize, m_gridSize, m_gridSize, c);
            }
        }
    }

    //std::cout << maxGrey << "\n";

    for (int x = 0; m_drawGrid && x <= (int)m_grid.width(); x++)
    {
        drawLine<float>(x*m_gridSize, 0, x*m_gridSize, m_grid.height() * m_gridSize, gridColor);
    }

    for (int y = 0; m_drawGrid && y <= (int)m_grid.height(); y++)
    {
        drawLine<float>(0, y*m_gridSize, m_grid.width() * m_gridSize, y*m_gridSize, gridColor);
    }

    m_game->window().draw(m_quadArray);
    m_game->window().draw(m_lineStrip);
    m_game->window().setView(m_game->window().getDefaultView());
    m_game->window().draw(m_text);
}

void Scene_Perlin2D::renderUI()
{
    const char vals[7] = { '.', 'G', '@', 'O', 'T', 'S', 'W' };
   
    ImGui::Begin("Options");

    if (ImGui::BeginTabBar("MyTabBar"))
    {
        if (ImGui::BeginTabItem("Perlin"))
        {
            if (ImGui::InputInt("Seed", &m_seed, 1, 1000))
            {
                calculateNoise();
            }

            if (ImGui::InputInt("SeedSize", &m_seedSize, 1, 100))
            {
                calculateNoise();
            }

            if (ImGui::InputInt("Octaves", &m_octaves, 1, 20))
            {
                calculateNoise();
            }

            if (ImGui::SliderFloat("Persistance", &m_persistance, 0, 2))
            {
                calculateNoise();
            }

            ImGui::SliderInt("Water Level", &m_waterLevel, 0, 255);
            

            // PC Display Options
            ImGui::Checkbox("Contours", &m_drawContours);
            ImGui::Checkbox("Greyscale", &m_drawGrey);

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}
