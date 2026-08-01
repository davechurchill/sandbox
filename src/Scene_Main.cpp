#include "Scene_Main.h"
#include "GameEngine.h"
#include "Profiler.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <string>

#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"

namespace
{
    ImGuiStyle DefaultUIStyle;
    float DefaultUIFontScale = 1.0f;
    bool DefaultUIStyleCaptured = false;
    constexpr const char * SettingsFile = "settings.json";

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
    m_session.processFrame(deltaTime);

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
        if (m_session.source()
            && (!mouseControlEvent || m_activeControlTab == ControlTab::Source))
        {
            m_session.source()->processEvent(event, m_mouseWorld);
        }
        if (m_session.processor() && !displayOpen)
        {
            if (m_activeControlTab == ControlTab::Overlay && m_session.overlay())
            {
                m_session.overlay()->processOverlayEvent(
                    event,
                    m_mouseWorld,
                    *m_session.processor());
            }
            else if (!mouseControlEvent || m_activeControlTab == ControlTab::Processor)
            {
                m_session.processor()->processEvent(event, m_mouseWorld);
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

            if (m_session.processor())
            {
                const bool mouseControlEvent = isMouseControlEvent(displayEvent);
                if (m_activeControlTab == ControlTab::Overlay && m_session.overlay())
                {
                    m_session.overlay()->processOverlayEvent(
                        displayEvent,
                        m_mouseDisplay,
                        *m_session.processor());
                }
                else if (!mouseControlEvent || m_activeControlTab == ControlTab::Processor)
                {
                    m_session.processor()->processEvent(displayEvent, m_mouseDisplay);
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

    if (m_session.source()) { m_session.source()->render(mainWindow()); }
    if (!m_session.processor()) { return; }
    sf::RenderWindow & target = m_game->displayWindow().isOpen()
        ? displayWindow()
        : mainWindow();
    m_session.processor()->render(target);
    if (m_session.overlay())
    {
        m_session.overlay()->renderOverlay(target, *m_session.processor());
    }
    m_session.processor()->projector().render(target);
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
        if (ImGui::BeginCombo("Selected Source", m_session.sourceID().c_str()))
        {
            for (const std::string & name : m_session.sourceNames())
            {
                bool selected = name == m_session.sourceID();
                if (ImGui::Selectable(name.c_str(), &selected))
                {
                    m_session.setSource(name);
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();
      
        if (m_session.source()) { m_session.source()->imgui(); }
      
        ImGui::EndTabItem();
    }

    // Processor

    if (ImGui::BeginTabItem("Processor"))
    {
        m_activeControlTab = ControlTab::Processor;
        if (ImGui::BeginCombo("Selected Processor", m_session.processorID().c_str()))
        {
            for (const std::string & name : m_session.processorNames())
            {
                bool selected = name == m_session.processorID();
                if (ImGui::Selectable(name.c_str(), &selected))
                {
                    m_session.setProcessor(name);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Separator();

        if (m_session.processor()) { m_session.processor()->imgui(); }

        ImGui::EndTabItem();
    }

    // Overlay

    if (ImGui::BeginTabItem("Overlay"))
    {
        m_activeControlTab = ControlTab::Overlay;
        if (ImGui::BeginCombo("Selected Overlay", m_session.overlayID().c_str()))
        {
            for (const std::string & name : m_session.overlayNames())
            {
                bool selected = name == m_session.overlayID();
                if (ImGui::Selectable(name.c_str(), &selected))
                {
                    m_session.setOverlay(name);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Separator();

        if (m_session.overlay())
        {
            if (m_session.overlay()->usesCanvasInput())
            {
                ImGui::TextWrapped("Left-click canvas input controls the selected overlay.");
            }
            m_session.overlay()->imguiOverlay();
        }

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::End();
}

void Scene_Main::save()
{
    PROFILE_FUNCTION();
    m_session.saveSettings(SettingsFile, m_doubleSizeUI);
}

void Scene_Main::load()
{
    PROFILE_FUNCTION();
    if (m_session.loadSettings(SettingsFile, m_doubleSizeUI))
    {
        applyUIScale();
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
    fout << "matrix" << m_session.topography();

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
