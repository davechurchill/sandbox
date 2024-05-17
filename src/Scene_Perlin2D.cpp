#include "Scene_Perlin2D.h"
#include "GameEngine.h"
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

    m_viewController.zoomTo(m_game->window(), 20, { 0, 0 });
        
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
    m_onContour = Grid<char>((int)(1 << m_seedSize), (int)(1 << m_seedSize), 0);
}

void Scene_Perlin2D::onFrame()
{
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
        m_viewController.processEvent(m_game->window(), event);

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
            if (event.mouseButton.button == sf::Mouse::Left) {}
            if (event.mouseButton.button == sf::Mouse::Right) {}
        }

        // happens when the mouse button is released
        if (event.type == sf::Event::MouseButtonReleased)
        {
            if (event.mouseButton.button == sf::Mouse::Left) { }
            if (event.mouseButton.button == sf::Mouse::Right) {}
        }

        // happens whenever the mouse is being moved
        if (event.type == sf::Event::MouseMoved)
        {
            m_mouseScreen = { (float)event.mouseMove.x, (float)event.mouseMove.y };

            // record the grid cell that the mouse position is currently over
            m_mouseGrid = { floor(m_mouseWorld.x / m_gridSize) * m_gridSize, floor(m_mouseWorld.y / m_gridSize) * m_gridSize };
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

    m_onContour = Grid<char>((int)(1 << m_seedSize), (int)(1 << m_seedSize), 0);

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

                float height = m_grid.get(x, y);
                if (height > m_contourLevel)
                {
                    if (y > 0 and m_grid.get(x, y - 1) <= m_contourLevel)
                    {
                        m_onContour.set(x, y, 1);
                    }

                    if (y < m_grid.height()-1 and m_grid.get(x, y + 1) <= m_contourLevel)
                    {
                        m_onContour.set(x, y, 1);
                    }

                    if (x > 0 and m_grid.get(x - 1, y) <= m_contourLevel)
                    {
                        m_onContour.set(x, y, 1);
                    }

                    if (x < m_grid.width()-1 and m_grid.get(x + 1, y) <= m_contourLevel)
                    {
                        m_onContour.set(x, y, 1);
                    }
                }



            }
        }
    }

    if (m_drawContours)
    {
        for (size_t x = 0; x < m_grid.width(); x++)
        {
            for (size_t y = 0; y < m_grid.height(); y++)
            {
                if (!m_onContour.get(x, y)) { continue; }

                float xx = x * m_gridSize + m_gridSize / 2;
                float yy = y * m_gridSize + m_gridSize / 2;

                if (y > 0 && m_onContour.get(x, y - 1))
                {
                    drawLine<float>(xx, yy, xx, yy - m_gridSize, sf::Color::White);
                }

                if (y < m_grid.height()-1 && m_onContour.get(x, y + 1))
                {
                    drawLine<float>(xx, yy, xx, yy + m_gridSize, sf::Color::White);
                }

                if (x > 0 && m_onContour.get(x - 1, y))
                {
                    drawLine<float>(xx, yy, xx - m_gridSize, yy, sf::Color::White);
                }

                if (x < m_grid.width() - 1 && m_onContour.get(x + 1, y))
                {
                    drawLine<float>(xx, yy, xx + m_gridSize, yy, sf::Color::White);
                }

                //

                if (x > 0 && y > 0 && m_onContour.get(x - 1, y - 1))
                {
                    drawLine<float>(xx, yy, xx - m_gridSize, yy - m_gridSize, sf::Color::White);
                }

                if (x > 0 && y < m_grid.height() - 1 && m_onContour.get(x - 1, y + 1))
                {
                    drawLine<float>(xx, yy, xx - m_gridSize, yy + m_gridSize, sf::Color::White);
                }

                if (x < m_grid.width() - 1 && y > 0 && m_onContour.get(x + 1, y - 1))
                {
                    drawLine<float>(xx, yy, xx + m_gridSize, yy - m_gridSize, sf::Color::White);
                }

                if (x < m_grid.width() - 1 && y < m_grid.height() - 1 && m_onContour.get(x + 1, y + 1))
                {
                    drawLine<float>(xx, yy, xx + m_gridSize, yy + m_gridSize, sf::Color::White);
                }
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

    if (m_drawContours)
    {

    }

    m_game->window().draw(m_quadArray);
    m_game->window().draw(m_lineStrip);
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
            ImGui::Checkbox("Greyscale", &m_drawGrey);
            ImGui::Checkbox("Contours", &m_drawContours);
            ImGui::SliderFloat("C Level", &m_contourLevel, 0, 1);
            ImGui::SliderInt("C Diff", &m_contourDiff, 1, 10);

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}
