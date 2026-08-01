#include "Scene_Main.h"
#include "GameEngine.h"
#include "Profiler.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <limits>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"

namespace
{
    ImGuiStyle DefaultUIStyle;
    float DefaultUIFontScale = 1.0f;
    bool DefaultUIStyleCaptured = false;
    constexpr const char * SettingsFile = "settings.json";

    struct DisplayTarget
    {
        sf::VideoMode mode;
        sf::Vector2i position;
    };

    struct MonitorOption
    {
        std::string id;
        std::string label;
    };

#if defined(_WIN32)
    BOOL CALLBACK collectMonitor(
        HMONITOR monitor,
        HDC,
        LPRECT,
        LPARAM monitorListAddress)
    {
        auto & monitors = *reinterpret_cast<std::vector<HMONITOR> *>(monitorListAddress);
        monitors.push_back(monitor);
        return TRUE;
    }

    std::vector<HMONITOR> getMonitors()
    {
        std::vector<HMONITOR> monitors;
        if (!EnumDisplayMonitors(
            nullptr,
            nullptr,
            collectMonitor,
            reinterpret_cast<LPARAM>(&monitors)))
        {
            monitors.clear();
        }
        return monitors;
    }

    bool getMonitorInfo(HMONITOR monitor, MONITORINFOEXA & info)
    {
        info = {};
        info.cbSize = sizeof(info);
        return GetMonitorInfoA(monitor, &info) != FALSE;
    }

    DisplayTarget makeDisplayTarget(const MONITORINFOEXA & info)
    {
        const LONG width = info.rcMonitor.right - info.rcMonitor.left;
        const LONG height = info.rcMonitor.bottom - info.rcMonitor.top;
        return {
            sf::VideoMode({ (unsigned int)width, (unsigned int)height }),
            { info.rcMonitor.left, info.rcMonitor.top }
        };
    }

    std::vector<MonitorOption> getMonitorOptions(const sf::Window & mainWindow)
    {
        std::vector<MonitorOption> options;
        const HMONITOR mainMonitor = MonitorFromWindow(
            mainWindow.getNativeHandle(),
            MONITOR_DEFAULTTOPRIMARY);

        for (HMONITOR monitor : getMonitors())
        {
            MONITORINFOEXA info{};
            if (!getMonitorInfo(monitor, info))
            {
                continue;
            }

            std::string displayName = info.szDevice;
            if (displayName.starts_with("\\\\.\\"))
            {
                displayName.erase(0, 4);
            }

            const bool isMain = monitor == mainMonitor;
            const bool isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
            std::string role;
            if (isMain && isPrimary) { role = " [Main, Primary]"; }
            else if (isMain) { role = " [Main]"; }
            else if (isPrimary) { role = " [Primary]"; }

            const LONG width = info.rcMonitor.right - info.rcMonitor.left;
            const LONG height = info.rcMonitor.bottom - info.rcMonitor.top;
            options.push_back({
                info.szDevice,
                std::format(
                    "{} - {}x{} at ({}, {}){}",
                    displayName,
                    width,
                    height,
                    info.rcMonitor.left,
                    info.rcMonitor.top,
                    role)
            });
        }
        return options;
    }

    DisplayTarget getDisplayTarget(
        const sf::Window & mainWindow,
        const std::string & selectedMonitorID)
    {
        DisplayTarget target{ sf::VideoMode::getDesktopMode(), { 0, 0 } };
        const HMONITOR mainMonitor = MonitorFromWindow(
            mainWindow.getNativeHandle(),
            MONITOR_DEFAULTTOPRIMARY);

        const std::vector<HMONITOR> monitors = getMonitors();
        if (!selectedMonitorID.empty())
        {
            for (HMONITOR monitor : monitors)
            {
                MONITORINFOEXA info{};
                if (getMonitorInfo(monitor, info)
                    && selectedMonitorID == info.szDevice)
                {
                    return makeDisplayTarget(info);
                }
            }
        }

        MONITORINFOEXA mainInfo{};
        if (!mainMonitor || !getMonitorInfo(mainMonitor, mainInfo))
        {
            return target;
        }

        HMONITOR selectedMonitor = mainMonitor;
        MONITORINFOEXA selectedInfo = mainInfo;
        long long closestDistance = std::numeric_limits<long long>::max();
        const long long mainCenterX = (long long)mainInfo.rcMonitor.left
            + mainInfo.rcMonitor.right;
        const long long mainCenterY = (long long)mainInfo.rcMonitor.top
            + mainInfo.rcMonitor.bottom;

        for (HMONITOR monitor : monitors)
        {
            if (monitor == mainMonitor)
            {
                continue;
            }

            MONITORINFOEXA info{};
            if (!getMonitorInfo(monitor, info))
            {
                continue;
            }

            const long long deltaX = (long long)info.rcMonitor.left
                + info.rcMonitor.right - mainCenterX;
            const long long deltaY = (long long)info.rcMonitor.top
                + info.rcMonitor.bottom - mainCenterY;
            const long long distance = deltaX * deltaX + deltaY * deltaY;
            if (distance < closestDistance)
            {
                closestDistance = distance;
                selectedMonitor = monitor;
                selectedInfo = info;
            }
        }

        return selectedMonitor ? makeDisplayTarget(selectedInfo) : target;
    }
#else
    std::vector<MonitorOption> getMonitorOptions(const sf::Window &)
    {
        const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        return {{
            "Desktop",
            std::format("Desktop - {}x{}", desktop.size.x, desktop.size.y)
        }};
    }

    DisplayTarget getDisplayTarget(const sf::Window &, const std::string &)
    {
        return { sf::VideoMode::getDesktopMode(), { 0, 0 } };
    }
#endif

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
            if (m_activeControlTab == ControlTab::Projection)
            {
                m_session.processor()->projector().processEvent(event, m_mouseWorld);
            }
            else if (m_activeControlTab == ControlTab::Overlay && m_session.overlay())
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
                if (m_activeControlTab == ControlTab::Projection)
                {
                    m_session.processor()->projector().processEvent(
                        displayEvent,
                        m_mouseDisplay);
                }
                else if (m_activeControlTab == ControlTab::Overlay && m_session.overlay())
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

    // Projection

    if (ImGui::BeginTabItem("Projection"))
    {
        m_activeControlTab = ControlTab::Projection;

        const std::vector<MonitorOption> monitorOptions = getMonitorOptions(m_game->window());
        std::string monitorPreview = "Automatic (nearest other monitor)";
        bool selectedMonitorAvailable = m_displayMonitorID.empty();
        for (const MonitorOption & option : monitorOptions)
        {
            if (option.id == m_displayMonitorID)
            {
                monitorPreview = option.label;
                selectedMonitorAvailable = true;
                break;
            }
        }
        if (!selectedMonitorAvailable)
        {
            monitorPreview = "Unavailable monitor (using Automatic)";
        }

        bool monitorChanged = false;
        if (ImGui::BeginCombo("Display Monitor", monitorPreview.c_str()))
        {
            if (ImGui::Selectable(
                "Automatic (nearest other monitor)",
                m_displayMonitorID.empty()))
            {
                monitorChanged = !m_displayMonitorID.empty();
                m_displayMonitorID.clear();
            }
            for (const MonitorOption & option : monitorOptions)
            {
                if (ImGui::Selectable(
                    option.label.c_str(),
                    option.id == m_displayMonitorID))
                {
                    monitorChanged = option.id != m_displayMonitorID;
                    m_displayMonitorID = option.id;
                }
            }
            ImGui::EndCombo();
        }

        if (monitorChanged && m_game->displayWindow().isOpen())
        {
            m_game->displayWindow().close();
            openDisplayWindow();
        }

        if (ImGui::Button("Toggle Display Window"))
        {
            toggleDisplayWindow();
        }
        ImGui::Separator();

        if (m_session.processor())
        {
            m_session.processor()->projector().imgui();
        }
        else
        {
            ImGui::TextUnformatted("Select a processor to configure its projection.");
        }

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::End();
}

void Scene_Main::save()
{
    PROFILE_FUNCTION();
    m_session.saveSettings(SettingsFile, m_doubleSizeUI, m_displayMonitorID);
}

void Scene_Main::load()
{
    PROFILE_FUNCTION();
    const std::string previousDisplayMonitorID = m_displayMonitorID;
    if (m_session.loadSettings(
        SettingsFile,
        m_doubleSizeUI,
        m_displayMonitorID))
    {
        applyUIScale();
        if (m_game->displayWindow().isOpen()
            && previousDisplayMonitorID != m_displayMonitorID)
        {
            m_game->displayWindow().close();
            openDisplayWindow();
        }
    }
}

void Scene_Main::toggleDisplayWindow()
{
    if (!m_game->displayWindow().isOpen())
    {
        openDisplayWindow();
    }
    else
    {
        m_game->displayWindow().close();
        m_switchWindows = false;
    }
}

void Scene_Main::openDisplayWindow()
{
    const DisplayTarget target = getDisplayTarget(
        m_game->window(),
        m_displayMonitorID);
    m_game->displayWindow().create(target.mode, "Display", sf::Style::None);
    m_game->displayWindow().setPosition(target.position);
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
