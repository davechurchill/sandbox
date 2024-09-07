#include "Scene_Main.h"
#include "Scene_Menu.h"
#include "GameEngine.h"
#include "Assets.h"
#include "Profiler.hpp"

#include "Processor_Colorizer.h"
#include "Source_Camera.h"
#include "Source_Perlin.h"
#include "Source_Snapshot.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"


Scene_Main::Scene_Main(GameEngine * game)
    : Scene(game)
{
    init();
}

void Scene_Main::init()
{
    ImGui::GetStyle().ScaleAllSizes(2.0f);
    ImGui::GetIO().FontGlobalScale = 2.0f;

    load();
    m_source->init();
    m_processor->init();
}

void Scene_Main::onFrame()
{
    m_topography = m_source->getTopography();
    if (m_topography.rows > 0 && m_topography.cols > 0)
    {
        m_processor->processTopography(m_topography);
    }

    sUserInput();
    sRender();
    if (m_drawUI)
    {
        renderUI();
    }
    m_currentFrame++;
}

// This method is for processing events that are independant of which window is selected
void Scene_Main::sProcessEvent(const sf::Event& event)
{
    // this event triggers when the window is closed
    if (event.type == sf::Event::Closed)
    {
        endScene();
        m_game->quit();
    }

    // this event is triggered when a key is pressed
    if (event.type == sf::Event::KeyPressed)
    {
        switch (event.key.code)
        {
        
        case sf::Keyboard::Escape:
        {
            endScene();
            break;
        }

        case sf::Keyboard::I:
        {
            m_drawUI = !m_drawUI;
            break;
        }

        case sf::Keyboard::F:
        {
            if (!m_game->displayWindow().isOpen())
            {
                m_game->displayWindow().create(sf::VideoMode(1920, 1080), "Display", sf::Style::None);
                m_game->displayWindow().setPosition({ -1920, 0 });
            }
            else
            {
                m_game->displayWindow().close();
            }
        }
        }
    }
}

void Scene_Main::sUserInput()
{
    PROFILE_FUNCTION();

    bool displayOpen = m_game->displayWindow().isOpen();

    sf::Event event;
    while (m_game->window().pollEvent(event))
    {
        ImGui::SFML::ProcessEvent(m_game->window(), event);
        m_viewController.processEvent(m_game->window(), event);
        sProcessEvent(event);

        // happens whenever the mouse is being moved
        if (event.type == sf::Event::MouseMoved)
        {
            m_mouseScreen = { event.mouseMove.x, event.mouseMove.y };
            m_mouseWorld = m_game->window().mapPixelToCoords(m_mouseScreen);
        }

        m_source->processEvent(event, m_mouseWorld);
        if (!displayOpen)
        {
            m_processor->processEvent(event, m_mouseWorld);
        }
    }

    if (displayOpen)
    {
        sf::Event displayEvent;
        while (m_game->displayWindow().pollEvent(displayEvent))
        {
            sProcessEvent(displayEvent);

            m_processor->processEvent(displayEvent, m_mouseDisplay);

            // happens whenever the mouse is being moved
            if (displayEvent.type == sf::Event::MouseMoved)
            {
                m_mouseDisplay = { (float)displayEvent.mouseMove.x, (float)displayEvent.mouseMove.y };
            }
        }
    }
}

// renders the scene
void Scene_Main::sRender()
{
    PROFILE_FUNCTION();

    m_game->window().clear();
    m_game->displayWindow().clear();

    m_source->render(m_game->window());
    if (m_game->displayWindow().isOpen())
    {
        m_processor->render(m_game->displayWindow());
    }
    else
    {
        m_processor->render(m_game->window());
    }
}

void Scene_Main::renderUI()
{
    PROFILE_FUNCTION();

    ImGui::BeginMainMenuBar();

    if (ImGui::Button("Snapshot"))
    {
        saveDataDump();
    }

    ImGui::Text("Framerate: %d", (int)m_game->framerate());
    ImGui::EndMainMenuBar();

    // Source

    ImGui::Begin("Source", &m_drawUI);

    const char * sources[] = {"Camera", "Perlin", "Snapshot"};
    if (ImGui::Combo("Selected Source", &m_sourceID, sources, IM_ARRAYSIZE(sources)))
    {
        setSource(m_sourceID);
    }

    ImGui::Separator();

    m_source->imgui();
    
    ImGui::End();

    // Processor

    ImGui::Begin("Processor", &m_drawUI);

    const char * processors[] = {"Colorizer"};
    if (ImGui::Combo("Selected Processor", &m_processorID, processors, IM_ARRAYSIZE(processors)))
    {
        setProcessor(m_processorID);
    }

    ImGui::Separator();

    m_processor->imgui();

    ImGui::End();
}

void Scene_Main::save()
{
    PROFILE_FUNCTION();
    std::ofstream current("currentSave.txt");
    current << m_saveFile << '\n';
    current.close();

    std::ofstream fout("saves/" + m_saveFile);

    fout << "m_sourceID " << m_sourceID << '\n';
    fout << "m_processorID " << m_processorID << '\n';

    m_source->save(fout);
    m_processor->save(fout);

    fout.close();
}

void Scene_Main::load()
{
    PROFILE_FUNCTION();
    std::ifstream current("currentSave.txt");
    if (current.good())
    {
        current >> m_saveFile;
    }
    current.close();

    std::string file = "saves/" + m_saveFile;

    // First find and initialize the source and processor
    std::ifstream fin(file);
    if (fin.good())
    {
        std::string temp;
        while (fin >> temp)
        {
            if (temp == "sourceID") { fin >> m_sourceID; }
            if (temp == "processorID") { fin >> m_processorID; }
        }
        fin.clear();
        fin.seekg(0, std::ios::beg);
    }

    // This initializes the source and processor, even if there was no save file
    setSource(m_sourceID);
    setProcessor(m_processorID);


    // Then load the settings for the source and processor
    if (fin.good())
    {
        std::string temp;
        while (fin >> temp)
        {
            m_source->load(temp, fin);
            m_processor->load(temp, fin);
        }
        fin.close();
    }
}

void Scene_Main::setSource(int source)
{
    m_sourceID = source;
    switch (source)
    {
    case TopographySource::Camera: { m_source = std::make_shared<Source_Camera>(); } break;
    case TopographySource::Perlin: { m_source = std::make_shared<Source_Perlin>(); } break;
    case TopographySource::Snapshot: { m_source = std::make_shared<Source_Snapshot>(); } break;
    }

    m_source->init();
}

void Scene_Main::setProcessor(int processor)
{
    m_processorID = processor;
    switch (processor)
    {
    case TopographyProcessor::Colorizer: { m_processor = std::make_shared<Processor_Colorizer>(); } break;
    }

    m_processor->init();
}

void Scene_Main::saveDataDump()
{
    cv::FileStorage fout("dataDumps/snapshot.bin", cv::FileStorage::WRITE);
    fout << "matrix" << m_topography;

}

void Scene_Main::endScene()
{
    m_game->changeScene<Scene_Menu>("Menu");
    m_game->displayWindow().close();
    save();
}