#include "SandboxGUI.h"
#include "Profiler.hpp"
#include "Tools.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <format>
#include <iostream>
#include <string>
#include <vector>

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

    bool eventMousePosition(const sf::Event & event, sf::Vector2i & position)
    {
        if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
        {
            position = moved->position;
            return true;
        }
        if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
        {
            position = pressed->position;
            return true;
        }
        if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>())
        {
            position = released->position;
            return true;
        }
        if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>())
        {
            position = wheel->position;
            return true;
        }
        return false;
    }
}


SandboxGUI::SandboxGUI()
    : m_window(sf::VideoMode({ 1600, 900 }), "Sandbox")
    , m_imguiInitialized(ImGui::SFML::Init(m_window))
{
    if (!m_imguiInitialized)
    {
        std::cerr << "Failed to initialize ImGui-SFML.\n";
        m_running = false;
        return;
    }
    if (!DefaultUIStyleCaptured)
    {
        DefaultUIStyle = ImGui::GetStyle();
        DefaultUIFontScale = ImGui::GetIO().FontGlobalScale;
        DefaultUIStyleCaptured = true;
    }
    applyUIScale();
}

void SandboxGUI::applyUIScale()
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

bool SandboxGUI::isRunning() const
{
    return m_running && m_window.isOpen();
}

void SandboxGUI::update()
{
    if (!isRunning()) { return; }

    const sf::Time dt = m_deltaClock.restart();
    m_framerate = m_framerate * 0.75f + 0.25f / dt.asSeconds();

    {
        PROFILE_SCOPE("ImGui::Update");
        ImGui::SFML::Update(m_window, dt);
    }

    const float deltaTime = dt.asMicroseconds() / 1000000.f;
    m_session.processFrame(deltaTime);

    sUserInput();
    sRender();
    if (m_drawUI)
    {
        renderUI();
    }

    ImGui::SFML::Render(m_window);

    {
        PROFILE_SCOPE("window.display()");
        m_window.display();

        if (m_displayWindow.isOpen())
        {
            m_displayWindow.display();
        }
    }
}

void SandboxGUI::run()
{
    load();
    while (isRunning())
    {
        update();
    }
}

void SandboxGUI::quit()
{
    m_displayWindow.close();
    m_running = false;
}

// This method is for processing events that are independant of which window is selected
void SandboxGUI::sProcessEvent(const sf::Event& event)
{
    // this event triggers when the window is closed
    if (event.is<sf::Event::Closed>())
    {
        quit();
    }

    // this event is triggered when a key is pressed
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>())
    {
        switch (keyPressed->code)
        {

        case sf::Keyboard::Key::Escape:
        {
            quit();
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

void SandboxGUI::routeControlEvent(
    const sf::Event & event,
    const sf::Vector2f & mouse,
    bool displayWindowEvent)
{
    const bool mouseControlEvent = isMouseControlEvent(event);
    switch (m_activeControlTab)
    {
    case ControlTab::Source:
    {
        if (displayWindowEvent
            && (!m_session.source().usesProjectedInput()
                || !m_session.projector().projectedDepthMapVisible()))
        {
            return;
        }

        sf::Vector2f sourceMouse = mouse;
        const bool mapped = !mouseControlEvent
            || !m_session.projector().projectedDepthMapVisible()
            || !m_session.source().usesProjectedInput()
            || m_session.projector().unprojectPoint(mouse, sourceMouse);
        if (mapped)
        {
            m_session.source().processEvent(event, sourceMouse);
        }
        break;
    }
    case ControlTab::Visualizer:
        if (TopographyVisualizer * visualizer = m_session.inputVisualizer())
        {
            visualizer->processEvent(event, mouse);
        }
        break;
    case ControlTab::Projection:
        m_session.projector().processEvent(event, mouse);
        break;
    }
}

void SandboxGUI::sUserInput()
{
    PROFILE_FUNCTION();

    const bool displayOpen = m_displayWindow.isOpen();
    auto & main = mainWindow();
    while (const auto polledEvent = main.pollEvent())
    {
        const sf::Event & event = *polledEvent;
        ImGui::SFML::ProcessEvent(main, event);
        const bool uiOwnsMouseWheel = event.is<sf::Event::MouseWheelScrolled>()
            && ImGui::GetIO().WantCaptureMouse;
        if (!uiOwnsMouseWheel)
        {
            m_viewController.processEvent(main, event);
        }
        sProcessEvent(event);

        sf::Vector2i eventPosition;
        if (eventMousePosition(event, eventPosition))
        {
            m_mouseScreen = eventPosition;
            m_mouseWorld = main.mapPixelToCoords(m_mouseScreen);
        }

        if (!displayOpen)
        {
            routeControlEvent(event, m_mouseWorld, false);
        }
        else if (m_activeControlTab == ControlTab::Source)
        {
            m_session.source().processEvent(event, m_mouseWorld);
        }
    }

    if (displayOpen)
    {
        auto & display = projectionWindow();
        while (const auto polledEvent = display.pollEvent())
        {
            const sf::Event & displayEvent = *polledEvent;
            sProcessEvent(displayEvent);

            sf::Vector2i eventPosition;
            if (eventMousePosition(displayEvent, eventPosition))
            {
                m_mouseDisplay = {
                    (float)eventPosition.x,
                    (float)eventPosition.y
                };
            }
            routeControlEvent(displayEvent, m_mouseDisplay, true);
        }
    }
}

// renders the scene
void SandboxGUI::sRender()
{
    PROFILE_FUNCTION();

    m_window.clear();
    m_displayWindow.clear();

    const bool displayOpen = m_displayWindow.isOpen();
    const bool showProjectedDepth = m_session.projector().projectedDepthMapVisible();
    if (displayOpen || !showProjectedDepth)
    {
        m_session.source().render(mainWindow());
    }

    sf::RenderWindow & target = displayOpen
        ? projectionWindow()
        : mainWindow();
    if (showProjectedDepth
        && m_session.projector().updateTexture(
            m_session.topography(),
            m_projectedDepthImage,
            m_projectedDepthSfImage,
            m_projectedDepthTexture,
            m_projectedDepthSprite,
            false,
            "Failed to load the projected depth-map texture.\n"))
    {
        m_projectedDepthSprite.setPosition(
            m_session.projector().getTransformedPosition());
        const float scale = m_session.projector().getTransformedScale();
        m_projectedDepthSprite.setScale({ scale, scale });
        target.draw(m_projectedDepthSprite);
    }
    if (m_session.projector().projectionVisible())
    {
        m_session.renderVisualizers(target);
    }
    m_session.projector().render(target);
}

void SandboxGUI::renderUI()
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
            if (m_displayWindow.isOpen() && ImGui::MenuItem("Switch windows"))
            {
                m_switchWindows = !m_switchWindows;
            }

            ImGui::EndMenu();
        }

        const std::string framerateText = std::to_string((int)m_framerate) + " fps";
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
        if (ImGui::BeginCombo("Selected Source", m_session.sourceName().data()))
        {
            for (const std::string & name : m_session.sourceNames())
            {
                bool selected = name == m_session.sourceName();
                if (ImGui::Selectable(name.c_str(), &selected))
                {
                    m_session.setSource(name);
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        m_session.source().imgui();

        ImGui::EndTabItem();
    }

    // Visualizer

    if (ImGui::BeginTabItem("Visualizer"))
    {
        m_activeControlTab = ControlTab::Visualizer;
        std::vector<std::string> visualizerNames = m_session.visualizerNames();
        std::sort(
            visualizerNames.begin(),
            visualizerNames.end(),
            [](const std::string & left, const std::string & right)
            {
                return std::lexicographical_compare(
                    left.begin(), left.end(),
                    right.begin(), right.end(),
                    [](unsigned char a, unsigned char b)
                    {
                        return std::tolower(a) < std::tolower(b);
                    });
            });

        for (const std::string & name : visualizerNames)
        {
            ImGui::PushID(name.c_str());

            bool enabled = m_session.visualizerEnabled(name);
            if (ImGui::Checkbox("##Enabled", &enabled))
            {
                m_session.setVisualizerEnabled(name, enabled);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip(enabled ? "Disable" : "Enable");
            }
            ImGui::SameLine();

            const bool wasExpanded = name == m_session.visualizerName();
            ImGui::SetNextItemOpen(wasExpanded, ImGuiCond_Always);
            const bool expanded = ImGui::CollapsingHeader(name.c_str());
            if (expanded != wasExpanded)
            {
                m_session.setVisualizer(expanded ? std::string_view(name) : std::string_view{});
            }

            if (expanded)
            {
                TopographyVisualizer * visualizer = m_session.visualizer();
                if (visualizer && visualizer->name() == name)
                {
                    ImGui::Indent();
                    if (visualizer->usesCanvasInput())
                    {
                        ImGui::TextWrapped("Canvas input controls this visualizer while it is expanded.");
                    }
                    ImGui::PushID("Options");
                    visualizer->imgui();
                    ImGui::PopID();
                    ImGui::Unindent();
                }
            }

            ImGui::PopID();
        }

        ImGui::EndTabItem();
    }

    // Projection

    if (ImGui::BeginTabItem("Projection"))
    {
        m_activeControlTab = ControlTab::Projection;

        if (Tools::imguiMonitorSelector(m_window, m_displayMonitorID)
            && m_displayWindow.isOpen())
        {
            m_displayWindow.close();
            Tools::openDisplayWindow(
                m_displayWindow,
                m_window,
                m_displayMonitorID);
        }

        if (ImGui::Button("Toggle Display Window"))
        {
            toggleDisplayWindow();
        }
        ImGui::Separator();

        m_session.projector().imgui();

        ImGui::EndTabItem();
    }

    ImGui::EndTabBar();
    ImGui::End();
}

void SandboxGUI::save()
{
    PROFILE_FUNCTION();
    m_session.saveSettings(SettingsFile, m_doubleSizeUI, m_displayMonitorID);
}

void SandboxGUI::load()
{
    PROFILE_FUNCTION();
    const std::string previousDisplayMonitorID = m_displayMonitorID;
    if (m_session.loadSettings(
        SettingsFile,
        m_doubleSizeUI,
        m_displayMonitorID))
    {
        applyUIScale();
        if (m_displayWindow.isOpen()
            && previousDisplayMonitorID != m_displayMonitorID)
        {
            m_displayWindow.close();
            Tools::openDisplayWindow(
                m_displayWindow,
                m_window,
                m_displayMonitorID);
        }
    }
}

void SandboxGUI::toggleDisplayWindow()
{
    if (!m_displayWindow.isOpen())
    {
        Tools::openDisplayWindow(
            m_displayWindow,
            m_window,
            m_displayMonitorID);
    }
    else
    {
        m_displayWindow.close();
        m_switchWindows = false;
    }
}

void SandboxGUI::saveDataDump()
{
    auto now = std::chrono::system_clock::now();
    cv::FileStorage fout(std::format("dataDumps/{0:%F_%H-%M-%S}_snapshot.bin", now), cv::FileStorage::WRITE);
    fout << "matrix" << m_session.topography();

}

sf::RenderWindow & SandboxGUI::mainWindow()
{
    return m_switchWindows ? m_displayWindow : m_window;
}

sf::RenderWindow & SandboxGUI::projectionWindow()
{
    return m_switchWindows ? m_window : m_displayWindow;
}
