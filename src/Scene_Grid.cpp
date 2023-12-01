#include "Scene_Grid.h"
#include "GameEngine.h"
#include "WorldView.hpp"
#include "Assets.h"
#include "Timer.hpp"
#include "Action.hpp"

#include "imgui.h"
#include "imgui-SFML.h"


#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <filesystem>

Scene_Grid::Scene_Grid(GameEngine * game)
    : Scene(game)
{
    init();
}

void Scene_Grid::init()
{
    m_view.setWindowSize(Vec2(m_game->window().getSize().x, m_game->window().getSize().y));

    m_view.setView(m_game->window().getView());
    m_view.zoomTo(8, { 0, 0 });
    //m_view.move({ -m_gridSize*3, -m_gridSize*3 });
        
    m_font = Assets::Instance().getFont("Tech");
    m_text.setFont(m_font);
    m_text.setPosition(10, 5);
    m_text.setCharacterSize(10);

    m_colors['.'] = sf::Color( 50, 200,  50);
    m_colors['G'] = sf::Color(  0, 220,   0);
    m_colors['@'] = sf::Color(  0,   0,   0);
    m_colors['O'] = sf::Color(  0,   0,   0);
    m_colors['T'] = sf::Color(128, 128, 128);
    m_colors['S'] = sf::Color(200,   0,   0);
    m_colors['W'] = sf::Color(  0, 102, 255);

    addMapFiles("maps");

    loadMap("maps/Default.map");
}

void Scene_Grid::addMapFiles(const std::string& dir)
{
    for (auto& p : std::filesystem::directory_iterator(dir))
    {
        std::string name = p.path().string();
        if (!p.is_directory()) { m_mapFiles.push_back(name); } 
    }

    for (auto& p : std::filesystem::directory_iterator(dir))
    {
        std::string name = p.path().string();
        if (p.is_directory()) { addMapFiles(p.path().string()); }
    }
}

void Scene_Grid::loadMap(const std::string& path)
{
    std::cout << "Loading Map: " << path << "\n";

    Timer t;
    m_env = Environment(path);
    auto ms = t.elapsed();
    std::cout << "            Load " << ms << "ms\n";

    t.start();
    m_connect = GridConnectivity(&m_env);
    ms = t.elapsed();
    std::cout << "    Connectivity " << ms << "ms\n";

    t.start();
    m_legalActions = GridLegalActions(m_env, m_connect, Actions8());
    ms = t.elapsed();
    std::cout << "         Actions " << ms << "ms\n";


    float mapDrawHeight = m_gridSize * m_env.height();
    float heightRatio = mapDrawHeight / m_game->window().getSize().y;

    m_view.setView(0, 0, 1600*heightRatio, mapDrawHeight);

    t.start();
    createGridTexture();
    ms = t.elapsed();
    std::cout << "         Texture " << ms << "ms\n";

    m_search = GridSearch(m_env);
}

void Scene_Grid::onFrame()
{
    m_view.update();
    sUserInput();
    
    Timer t;
    bool ran = false;
    if (m_search.isRunning())
    {
        ran = true;
        m_search.search();
    }
    long long elapsed = t.elapsed();
    if (ran) { m_lastSearch = t.elapsed(); }

    sRender(); 
    m_currentFrame++;
}

void Scene_Grid::sUserInput()
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
                case sf::Keyboard::W: m_view.zoom(0.8); break;
                case sf::Keyboard::D: m_view.zoom(1.25); break;
                case sf::Keyboard::G: m_drawGrid = !m_drawGrid; break;
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

        if (ImGui::GetIO().WantCaptureMouse) { continue; }

        if (event.type == sf::Event::MouseButtonPressed)
        {
            // happens when the left mouse button is pressed
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                m_mouseButton[0] = true;
                m_view.stopScroll();
                
                if (!m_env.isOOB(m_mouseCell))
                {
                    if (m_startCell.x == -1)
                    {
                        m_startCell = m_mouseCell;
                    }
                    else
                    {
                        m_goalCell = m_mouseCell;
                        m_search.startSearch(m_startCell, m_goalCell);
                    }
                    
                }
            }

            // happens when the right mouse button is pressed
            if (event.mouseButton.button == sf::Mouse::Right)
            {
                m_mouseButton[2] = true;
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
                m_mouseButton[0] = false;
                if (m_goalCell.x != -1)
                {
                    m_startCell = { -1, -1 };
                    m_goalCell = { -1, -1 };
                }
            }

            // let go of the currently selected rectangle
            if (event.mouseButton.button == sf::Mouse::Right)
            {
                m_mouseButton[2] = false;
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

            m_mouseCell.x = (int)m_mouseWorld.x / (int)m_gridSize;
            m_mouseCell.y = (int)m_mouseWorld.y / (int)m_gridSize;

            if (m_mouseButton[0] && m_startCell.x != -1)
            {
                if (m_goalCell != m_mouseCell)
                {
                    m_goalCell = m_mouseCell;
                    m_search.startSearch(m_startCell, m_goalCell);
                }
                
            }

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

void Scene_Grid::createGridTexture()
{
    m_gridImage.create((uint32_t)m_env.width(), (uint32_t)m_env.height());

    for (uint32_t x = 0; x < m_env.width(); x++)
    {
        for (uint32_t y = 0; y < m_env.height(); y++)
        {
            m_gridImage.setPixel(x, y, m_colors[m_env.get(State(x, y))]);
        }
    }

    m_gridTexture.loadFromImage(m_gridImage);
    m_gridSprite = sf::Sprite(m_gridTexture);
    m_gridSprite.setScale(m_gridSize, m_gridSize);
}

// renders the scene
void Scene_Grid::sRender()
{
    const sf::Color gridColor(32, 32, 32);

    m_game->window().clear(sf::Color(10, 10, 10));
    m_lineStrip.clear();
    m_quadArray.clear();
    float gs = (float)m_gridSize;
    m_game->window().setView(m_view.getSFMLView());
    
    // draw the grid texture
    m_game->window().draw(m_gridSprite);

    // draw connected cells
    if (false && m_mouseButton[2] && !m_env.isOOB(m_mouseCell))
    {
        for (uint32_t x = 0; x < m_env.width(); x++)
        {
            for (uint32_t y = 0; y < m_env.height(); y++)
            {
                if (m_connect.isConnected(State(x, y), m_mouseCell))
                {
                    drawRect<float>(x * m_gridSize, y * m_gridSize, m_gridSize, m_gridSize, sf::Color(255, 0, 255));
                }
            }
        }
    }

    for (int x = 0; m_drawGrid && x <= (int)m_env.width(); x++)
    {
        drawLine<float>(x*m_gridSize, 0, x*m_gridSize, m_env.height() * m_gridSize, gridColor);
    }

    for (int y = 0; m_drawGrid && y <= (int)m_env.height(); y++)
    {
        drawLine<float>(0, y*m_gridSize, m_env.width() * m_gridSize, y*m_gridSize, gridColor);
    }

    auto open = m_search.getOpen();
    for (auto cell : open)
    {
        drawRect<float>(cell.x * m_gridSize, cell.y * m_gridSize, m_gridSize, m_gridSize, sf::Color(255, 255, 0));
    }

    auto closed = m_search.getClosed();
    for (auto cell : closed)
    {
        drawRect<float>(cell.x * m_gridSize, cell.y * m_gridSize, m_gridSize, m_gridSize, sf::Color(255, 0, 0));
    }

    for (auto cell : m_search.getPath())
    {
        drawRect<float>(cell.x * m_gridSize, cell.y * m_gridSize, m_gridSize, m_gridSize, sf::Color(255, 255, 255));
    }
    

    drawRect<float>(m_mouseCell.x * m_gridSize, m_mouseCell.y * m_gridSize, m_gridSize, m_gridSize, sf::Color(255, 255, 255));

    m_game->window().draw(m_quadArray);
    m_game->window().draw(m_lineStrip);
    m_game->window().setView(m_game->window().getDefaultView());
    m_game->window().draw(m_text);

    renderUI();

    //m_game->window().display();
}

void Scene_Grid::renderUI()
{
    const char vals[7] = { '.', 'G', '@', 'O', 'T', 'S', 'W' };
    size_t size = m_env.width() * m_env.height();

    ImGui::Begin("Map Options");

    if (ImGui::BeginTabBar("MyTabBar"))
    {
        if (ImGui::BeginTabItem("Map Info"))
        {
            ImGui::Text("Mouse:  %d %d", m_mouseCell.x, m_mouseCell.y);
            ImGui::Text("Button: %d %d", m_mouseButton[0], m_mouseButton[2]);
            ImGui::Text("Timer: %llu", m_lastSearch);
            ImGui::Separator();
            ImGui::Text("File:   %s", m_env.getPath().c_str());
            ImGui::Text("Width:  %d", m_env.width());
            ImGui::Text("Height: %d", m_env.height());
            ImGui::Text("Size:   %d kb", (size*4)/1024);
            ImGui::Separator();
            ImGui::Text("%Value    Count    Percent");
            for (size_t i = 0; i < 7; i++)
            {
                ImGui::Text("%c     %8llu      %5.2lf", vals[i], m_env.getCount(vals[i]), (double)m_env.getCount(vals[i])/size*100.0);
            }
            ImGui::Separator();
            if (ImGui::BeginListBox("##listboxMaps", ImVec2(-FLT_MIN, 8 * ImGui::GetTextLineHeightWithSpacing())))
            {
                for (auto& map : m_mapFiles)
                {
                    if (ImGui::Selectable(map.c_str(), false))
                    {
                        loadMap(map);
                    }
                }
                ImGui::EndListBox();
            }

            // PC Display Options
            ImGui::Checkbox("Grid  ", &m_drawGrid);

            ImGui::EndTabItem();
        } 
        ImGui::EndTabBar();
    }

    ImGui::End();
}