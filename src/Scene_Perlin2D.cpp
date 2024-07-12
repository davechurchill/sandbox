#include "Scene_Perlin2D.h"
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

    m_shader.loadFromFile("shaders/shader_contour_color.frag", sf::Shader::Fragment);

    calculateNoise();
}

void Scene_Perlin2D::calculateNoise()
{
    m_perlin = Perlin2DNew((int)(1 << m_seedSize), (int)(1 << m_seedSize), m_seed);
    m_grid = m_perlin.GeneratePerlinNoise(m_octaves, m_persistance);
}

void Scene_Perlin2D::imageFromGrid()
{
    m_image.create((int)m_grid.width(), (int)m_grid.height());
    for (unsigned int i = 0; i < m_grid.width(); ++i)
    {
        for (unsigned int j = 0; j < m_grid.height(); ++j)
        {
            sf::Uint8 c = (sf::Uint8)(m_grid.get(i, j) * 255.f);
            m_image.setPixel(i, j, sf::Color(c,c,c));
        }
    }
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
                    m_game->changeScene<Scene_Menu>("Menu");
                    break;
                }
                case sf::Keyboard::R: { m_seed += 1; calculateNoise(); break; }
                case sf::Keyboard::W: { m_octaves = std::min(m_octaves + 1, m_seedSize); calculateNoise();  break; }
                case sf::Keyboard::S: { m_octaves--; calculateNoise();  break; }
                case sf::Keyboard::A: { m_persistance -= 0.1f; if (m_persistance < 0.1f) { m_persistance = 0.1f; } calculateNoise();  break; }
                case sf::Keyboard::D: { m_persistance += 0.1f; calculateNoise();  break; }
                case sf::Keyboard::G: { m_drawGrey = !m_drawGrey; break; }
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

    m_game->window().draw(m_quadArray);
    m_game->window().draw(m_lineStrip);
    m_game->window().draw(m_text);

    imageFromGrid();
    m_texture.loadFromImage(m_image);
    m_sprite.setTexture(m_texture, true);
    m_sprite.setScale(m_gridSize, m_gridSize);

    m_shader.setUniform("shaderIndex", m_selectedShaderIndex);
    m_shader.setUniform("contour", m_drawContours);
    m_shader.setUniform("numberOfContourLines", m_numberOfContourLines);

    m_game->window().draw(m_sprite, &m_shader);
}

void Scene_Perlin2D::renderUI()
{  
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
            

            // PC Display Options
            const char * shaders[] = { "Popsicle", "Red", "Terrain", "None" };
            ImGui::Combo("Color Scheme", &m_selectedShaderIndex, shaders, 4);

            ImGui::Checkbox("Contours", &m_drawContours);

            ImGui::InputInt("Contour Lines", &m_numberOfContourLines, 1, 10);

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Minecraft"))
        {
            //m_game->minecraft().imgui(m_grid);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}