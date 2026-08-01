#include "Scene_Main.h"
#include "GameEngine.h"
#include "Profiler.hpp"

#include "Processor_Balls.h"
#include "Overlay_BFS.h"
#include "Overlay_Cloth.h"
#include "Overlay_CloudSimulation.h"
#include "Overlay_SmokeFire.h"
#include "Overlay_Weather.h"
#include "Processor_Colorizer.h"
#include "Processor_FishPond.h"
#include "Processor_ForestFire.h"
#include "Processor_Heat.h"
#include "Processor_Minecraft.h"
#include "Processor_Nature.h"
#include "Processor_TerrainLighting.h"
#include "Processor_Vectors.h"
#include "Processor_WaterFlow.h"
#include "Source_Camera.h"
#include "Source_PaintBrush.h"
#include "Source_Perlin.h"
#include "Source_Snapshot.h"
#include "Source_Waves.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>
#include <chrono>

#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"

namespace
{
    ImGuiStyle DefaultUIStyle;
    float DefaultUIFontScale = 1.0f;
    bool DefaultUIStyleCaptured = false;

    bool isMouseControlEvent(const sf::Event & event)
    {
        return event.is<sf::Event::MouseMoved>()
            || event.is<sf::Event::MouseButtonPressed>()
            || event.is<sf::Event::MouseButtonReleased>()
            || event.is<sf::Event::MouseWheelScrolled>()
            || event.is<sf::Event::MouseEntered>()
            || event.is<sf::Event::MouseLeft>();
    }
}


Scene_Main::Scene_Main(GameEngine * game)
    : Scene(game)
{
    init();
}

void Scene_Main::init()
{
    if (!DefaultUIStyleCaptured)
    {
        DefaultUIStyle = ImGui::GetStyle();
        DefaultUIFontScale = ImGui::GetIO().FontGlobalScale;
        DefaultUIStyleCaptured = true;
    }
    applyUIScale();

    registerSource<Source_Camera>("Camera");
    registerSource<Source_PaintBrush>("PaintBrush");
    registerSource<Source_Perlin>("Perlin");
    registerSource<Source_Snapshot>("Snapshot");
    registerSource<Source_Waves>("Waves");

    registerProcessor<Processor_Colorizer>("Colorizer");
    registerProcessor<Processor_FishPond>("Fish Pond");
    registerProcessor<Processor_ForestFire>("Forest Fire");
    registerProcessor<Processor_Heat>("Heat");
    registerProcessor<Processor_Minecraft>("Minecraft");
    registerProcessor<Processor_Nature>("Nature");
    registerProcessor<Processor_TerrainLighting>("TerrainLighting");
    m_processorMap.emplace("None", []() {return nullptr; });

    registerOverlay<Processor_Nature>("Animals");
    registerOverlay<Overlay_BFS>("BFS");
    registerOverlay<Processor_Balls>("Balls");
    registerOverlay<Overlay_Cloth>("Cloth Sheet");
    registerOverlay<Overlay_CloudSimulation>("Cloud Simulation");
    registerOverlay<Overlay_SmokeFire>("Smoke and Fire");
    registerOverlay<Processor_Vectors>("Vectors");
    registerOverlay<Overlay_Weather>("Weather");
    registerOverlay<Processor_WaterFlow>("WaterFlow");
    m_overlayMap.emplace("None", []() {return nullptr; });
}

void Scene_Main::applyUIScale()
{
    if (!DefaultUIStyleCaptured)
    {
        return;
    }

    const float scale = m_doubleSizeUI ? 2.0f : 1.0f;
    ImGui::GetStyle() = DefaultUIStyle;
    ImGui::GetStyle().ScaleAllSizes(scale);
    ImGui::GetIO().FontGlobalScale = DefaultUIFontScale * scale;
}

void Scene_Main::onFrame(float deltaTime)
{
    m_topography = m_source->getTopography();
    if (m_processor && m_topography.rows > 0 && m_topography.cols > 0)
    {
        IntermediateData data;
        data.deltaTime = deltaTime;
        data.topography = m_topography;
        data.markers = m_source->getMarkers();
        m_processor->processTopography(data);
        if (m_overlay)
        {
            m_overlay->processTopographyOverlay(data, *m_processor);
        }
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
    if (event.is<sf::Event::Closed>())
    {
        endScene();
        m_game->quit();
    }

    // this event is triggered when a key is pressed
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        switch (keyPressed->code)
        {
        
        case sf::Keyboard::Key::Escape:
        {
            endScene();
            break;
        }

        case sf::Keyboard::Key::I:
        {
            m_drawUI = !m_drawUI;
            break;
        }

        case sf::Keyboard::Key::F:
        {
            toggleDisplayWindow();
        }
        }
    }
}

void Scene_Main::sUserInput()
{
    PROFILE_FUNCTION();

    bool displayOpen = m_game->displayWindow().isOpen();
    auto & main = mainWindow();
    while (const auto polledEvent = main.pollEvent())
    {
        const sf::Event& event = *polledEvent;
        ImGui::SFML::ProcessEvent(main, event);
        const bool uiOwnsMouseWheel = event.is<sf::Event::MouseWheelScrolled>()
            && ImGui::GetIO().WantCaptureMouse;
        if (!uiOwnsMouseWheel)
        {
            m_viewController.processEvent(main, event);
        }
        sProcessEvent(event);

        // happens whenever the mouse is being moved
        if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
        {
            m_mouseScreen = mouseMoved->position;
            m_mouseWorld = main.mapPixelToCoords(m_mouseScreen);
        }

        const bool mouseControlEvent = isMouseControlEvent(event);
        if (m_source
            && (!mouseControlEvent || m_activeControlTab == ControlTab::Source))
        {
            m_source->processEvent(event, m_mouseWorld);
        }
        if (m_processor && !displayOpen)
        {
            if (m_activeControlTab == ControlTab::Overlay && m_overlay)
            {
                m_overlay->processOverlayEvent(event, m_mouseWorld, *m_processor);
            }
            else if (!mouseControlEvent || m_activeControlTab == ControlTab::Processor)
            {
                m_processor->processEvent(event, m_mouseWorld);
            }
        }
    }

    if (displayOpen)
    {
        auto & display = displayWindow();
        while (const auto polledEvent = display.pollEvent())
        {
            const sf::Event& displayEvent = *polledEvent;
            sProcessEvent(displayEvent);

            if (m_processor)
            {
                const bool mouseControlEvent = isMouseControlEvent(displayEvent);
                if (m_activeControlTab == ControlTab::Overlay && m_overlay)
                {
                    m_overlay->processOverlayEvent(displayEvent, m_mouseDisplay, *m_processor);
                }
                else if (!mouseControlEvent || m_activeControlTab == ControlTab::Processor)
                {
                    m_processor->processEvent(displayEvent, m_mouseDisplay);
                }
            }

            // happens whenever the mouse is being moved
            if (const auto* mouseMoved = displayEvent.getIf<sf::Event::MouseMoved>())
            {
                m_mouseDisplay = { (float)mouseMoved->position.x, (float)mouseMoved->position.y };
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

    if (m_source) { m_source->render(mainWindow()); }
    if (!m_processor) { return; }
    sf::RenderWindow & target = m_game->displayWindow().isOpen()
        ? displayWindow()
        : mainWindow();
    m_processor->render(target);
    if (m_overlay)
    {
        m_overlay->renderOverlay(target, *m_processor);
    }
    m_processor->projector().render(target);
}

void Scene_Main::renderUI()
{
    PROFILE_FUNCTION();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Menu"))
        {
            if (ImGui::BeginMenu("UI Size"))
            {
                if (ImGui::MenuItem("Default", nullptr, !m_doubleSizeUI))
                {
                    m_doubleSizeUI = false;
                    applyUIScale();
                }
                if (ImGui::MenuItem("Double", nullptr, m_doubleSizeUI))
                {
                    m_doubleSizeUI = true;
                    applyUIScale();
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Toggle Display Window", "F"))
            {
                toggleDisplayWindow();
            }
            if (ImGui::MenuItem("Save Settings"))
            {
                save();
            }
            if (ImGui::MenuItem("Load Settings"))
            {
                load();
            }
            if (ImGui::MenuItem("Snapshot"))
            {
                saveDataDump();
            }
            if (m_game->displayWindow().isOpen() && ImGui::MenuItem("Switch windows"))
            {
                m_switchWindows = !m_switchWindows;
            }

            ImGui::EndMenu();
        }

        const std::string framerateText = std::to_string((int)m_game->framerate()) + " fps";
        const float rightAlignedX = ImGui::GetWindowContentRegionMax().x
            - ImGui::CalcTextSize(framerateText.c_str()).x;
        ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), rightAlignedX));
        ImGui::TextUnformatted(framerateText.c_str());

        ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Controls", &m_drawUI);
    ImGui::BeginTabBar("ControlTabs");

    // Source

    if (ImGui::BeginTabItem("Source"))
    {
        m_activeControlTab = ControlTab::Source;
        if (ImGui::BeginCombo("Selected Source", m_sourceID.c_str()))
        {
            for (auto & [name, _] : m_sourceMap)
            {
                bool selected = name == m_sourceID;
                if (ImGui::Selectable(name.c_str(), &selected))
                {
                    setSource(name);
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();
      
        if (m_source) { m_source->imgui(); }
      
        ImGui::EndTabItem();
    }

    // Processor

    if (ImGui::BeginTabItem("Processor"))
    {
        m_activeControlTab = ControlTab::Processor;
        if (ImGui::BeginCombo("Selected Processor", m_processorID.c_str()))
        {
            for (auto & [name, _] : m_processorMap)
            {
                bool selected = name == m_processorID;
                if (ImGui::Selectable(name.c_str(), &selected))
                {
                    setProcessor(name);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Separator();

        if (m_processor) { m_processor->imgui(); }

        ImGui::EndTabItem();
    }

    // Overlay

    if (ImGui::BeginTabItem("Overlay"))
    {
        m_activeControlTab = ControlTab::Overlay;
        if (ImGui::BeginCombo("Selected Overlay", m_overlayID.c_str()))
        {
            for (auto & [name, _] : m_overlayMap)
            {
                bool selected = name == m_overlayID;
                if (ImGui::Selectable(name.c_str(), &selected))
                {
                    setOverlay(name);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Separator();

        if (m_overlay)
        {
            if (m_overlay->usesCanvasInput())
            {
                ImGui::TextWrapped("Left-click canvas input controls the selected overlay.");
            }
            m_overlay->imguiOverlay();
        }

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::End();
}

void Scene_Main::save()
{
    PROFILE_FUNCTION();
    std::ofstream current("currentSave.txt");
    current << m_saveFile << '\n';
    current.close();

    if (m_source) { m_source->save(m_save); }
    if (m_processor) { m_processor->save(m_save); }
    if (m_overlay) { m_overlay->saveOverlay(m_save); }

    m_save.source = m_sourceID;
    m_save.processor = m_processorID;
    m_save.overlay = m_overlayID;
    m_save.doubleSizeUI = m_doubleSizeUI;

    m_save.saveToFile("saves/" + m_saveFile);
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
    m_save.overlay = "None";
    m_save.loadFromFile(file);
    m_doubleSizeUI = m_save.doubleSizeUI;
    applyUIScale();

    // Migrate saves for processors that moved to overlays.
    if (m_save.processor == "Balls")
    {
        m_save.processor = "Colorizer";
        m_save.overlay = "Balls";
    }
    if (m_save.processor == "WaterFlow")
    {
        m_save.processor = "Colorizer";
        m_save.overlay = "WaterFlow";
    }
    if (m_save.processor == "Vectors")
    {
        m_save.processor = "Colorizer";
        m_save.overlay = "Vectors";
    }
    if (m_save.overlay == "Lava Rocks")
    {
        m_save.overlay = "Balls";
    }

    // This initializes the source and processor, even if there was no save file
    setSource(m_save.source);
    setProcessor(m_save.processor);
    setOverlay(m_save.overlay);
}

void Scene_Main::setSource(const std::string & source)
{
    const bool sourceChanged = !m_source || source != m_sourceID;
    if (m_source) { m_source->save(m_save); }
    m_sourceID = source;
    if (m_sourceMap.contains(source))
    {
        m_source = m_sourceMap.at(source)();
    }
    else
    {
        m_source = m_sourceMap.at("Camera")();
    }
    if (m_source) 
    {
        m_source->init();
        m_source->load(m_save);
    }
    if (sourceChanged && m_processor)
    {
        m_processor->onSourceChanged();
    }
}

void Scene_Main::setProcessor(const std::string & processor)
{
    if (m_processor) { m_processor->save(m_save); }
    m_processorID = processor;
    if (m_processorMap.contains(processor))
    {
        m_processor = m_processorMap.at(processor)();
    }
    else
    {
        m_processor = m_processorMap.at("Colorizer")();
    }
    if (m_processor) 
    {
        m_processor->init();
        m_processor->load(m_save);
    }
}

void Scene_Main::setOverlay(const std::string & overlay)
{
    if (m_overlay) { m_overlay->saveOverlay(m_save); }
    m_overlayID = overlay;
    if (m_overlayMap.contains(overlay))
    {
        m_overlay = m_overlayMap.at(overlay)();
    }
    else
    {
        m_overlayID = "None";
        m_overlay = m_overlayMap.at("None")();
    }
    if (m_overlay)
    {
        m_overlay->initOverlay();
        m_overlay->loadOverlay(m_save);
    }
}

void Scene_Main::toggleDisplayWindow()
{
    if (!m_game->displayWindow().isOpen())
    {
        m_game->displayWindow().create(sf::VideoMode({ 1920, 1080 }), "Display", sf::Style::None);
        m_game->displayWindow().setPosition({ -1920, 0 });
    }
    else
    {
        m_game->displayWindow().close();
        m_switchWindows = false;
    }
}

void Scene_Main::saveDataDump()
{
    auto now = std::chrono::system_clock::now();
    cv::FileStorage fout(std::format("dataDumps/{0:%F_%H-%M-%S}_snapshot.bin", now), cv::FileStorage::WRITE);
    fout << "matrix" << m_topography;

}

sf::RenderWindow & Scene_Main::mainWindow()
{
    return m_switchWindows ? m_game->displayWindow() : m_game->window();
}

sf::RenderWindow & Scene_Main::displayWindow()
{
    return m_switchWindows ? m_game->window() : m_game->displayWindow();
}

void Scene_Main::endScene()
{
    m_game->displayWindow().close();
    m_game->quit();
}
