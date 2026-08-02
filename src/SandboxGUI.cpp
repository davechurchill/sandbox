#include "SandboxGUI.h"
#include "Profiler.hpp"
#include "Tools.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace
{
    ImGuiStyle DefaultUIStyle;
    float DefaultUIFontScale = 1.0f;
    bool DefaultUIStyleCaptured = false;

    std::filesystem::path ApplicationDirectory()
    {
#if defined(_WIN32)
        std::vector<wchar_t> path(32768);
        const DWORD length = GetModuleFileNameW(nullptr, path.data(), (DWORD)path.size());
        if (length > 0 && length < path.size())
        {
            return std::filesystem::path(std::wstring(path.data(), length)).parent_path();
        }
#endif
        std::error_code error;
        const std::filesystem::path currentDirectory = std::filesystem::current_path(error);
        return error ? std::filesystem::path(".") : currentDirectory;
    }

    const std::filesystem::path SettingsDirectory = ApplicationDirectory() / "settings";
    const std::filesystem::path QuickSettingsFile = SettingsDirectory / "settings.json";

    std::string Lowercase(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character)
        {
            return (char)std::tolower(character);
        });
        return value;
    }

    bool HasJsonExtension(const std::filesystem::path & path)
    {
        return Lowercase(path.extension().string()) == ".json";
    }

    std::string TrimWhitespace(std::string value)
    {
        const auto notWhitespace = [](unsigned char character)
        {
            return !std::isspace(character);
        };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(), notWhitespace));
        value.erase(std::find_if(value.rbegin(), value.rend(), notWhitespace).base(), value.end());
        return value;
    }

    std::optional<std::filesystem::path> BuildSettingsPath(const char * input, std::string & error)
    {
        std::string name = TrimWhitespace(input ? input : "");
        if (name.empty())
        {
            error = "Enter a filename before saving.";
            return std::nullopt;
        }

        const std::string lowercaseName = Lowercase(name);
        if (lowercaseName.ends_with(".json"))
        {
            name.resize(name.size() - 5);
            name = TrimWhitespace(name);
        }

        constexpr std::string_view InvalidCharacters = "<>:\"/\\|?*";
        const bool invalidCharacter = std::any_of(name.begin(), name.end(), [InvalidCharacters](unsigned char character)
        {
            return character < 32 || InvalidCharacters.find((char)character) != std::string_view::npos;
        });
        if (name.empty() || name == "." || name == ".." || name.back() == '.' || invalidCharacter)
        {
            error = "Use a simple filename without paths or special characters.";
            return std::nullopt;
        }

        error.clear();
        return SettingsDirectory / (name + ".json");
    }

    bool IsMouseControlEvent(const sf::Event & event)
    {
        return event.is<sf::Event::MouseMoved>()
            || event.is<sf::Event::MouseButtonPressed>()
            || event.is<sf::Event::MouseButtonReleased>()
            || event.is<sf::Event::MouseWheelScrolled>()
            || event.is<sf::Event::MouseEntered>()
            || event.is<sf::Event::MouseLeft>();
    }

    bool EventMousePosition(const sf::Event & event, sf::Vector2i & position)
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
    sUserInput();
    if (!isRunning()) { return; }

    m_session.processFrame(deltaTime);
    frameInitialView();

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

    m_terrain3DView.update(m_session.topography());
}

void SandboxGUI::frameInitialView()
{
    const cv::Mat & topography = m_session.topography();
    if (!m_initialViewPending || topography.empty()) { return; }

    sf::Vector2f minimum{ 0.0f, 0.0f };
    sf::Vector2f maximum{ (float)topography.cols, (float)topography.rows };
    const sf::FloatRect projectionBounds = m_session.projector().projectionBounds();
    minimum.x = std::min(minimum.x, projectionBounds.position.x);
    minimum.y = std::min(minimum.y, projectionBounds.position.y);
    maximum.x = std::max(maximum.x, projectionBounds.position.x + projectionBounds.size.x);
    maximum.y = std::max(maximum.y, projectionBounds.position.y + projectionBounds.size.y);

    m_viewController.frameBounds(
        m_window,
        { minimum, maximum - minimum },
        0.10f);
    m_initialViewPending = false;
}

void SandboxGUI::run()
{
    quickLoadSettings();
    while (isRunning())
    {
        update();
    }
}

void SandboxGUI::quit()
{
    m_terrain3DView.close();
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
    const bool mouseControlEvent = IsMouseControlEvent(event);
    if (!displayWindowEvent && mouseControlEvent && ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

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
        if (Visualizer * visualizer = m_session.inputVisualizer())
        {
            visualizer->processEvent(event, mouse);
        }
        break;
    case ControlTab::Settings:
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
        if (EventMousePosition(event, eventPosition))
        {
            m_mouseScreen = eventPosition;
            m_mouseWorld = main.mapPixelToCoords(m_mouseScreen);
        }

        if (!displayOpen)
        {
            routeControlEvent(event, m_mouseWorld, false);
        }
        else if (m_activeControlTab == ControlTab::Source
            && (!IsMouseControlEvent(event) || !ImGui::GetIO().WantCaptureMouse))
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
            if (EventMousePosition(displayEvent, eventPosition))
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
    if (showProjectedDepth)
    {
        m_session.projector().drawTerrain(target);
    }
    SandboxProjector & projector = m_session.projector();
    if (projector.projectionVisible())
    {
        m_session.renderVisualizers(target);
        if (m_terrain3DView.isOpen())
        {
            m_terrain3DView.captureVisualization(
                m_session.topography(),
                target,
                projector.getProjectionMatrix(),
                projector.getTransformedPosition(),
                projector.getTransformedScale());
        }
    }
    projector.render(target);
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
            if (ImGui::MenuItem(
                "3D Terrain View",
                nullptr,
                m_terrain3DView.isOpen()))
            {
                m_terrain3DView.toggle();
            }
            if (ImGui::MenuItem("Quick Save Settings"))
            {
                quickSaveSettings();
            }
            if (ImGui::MenuItem("Quick Load Settings"))
            {
                quickLoadSettings();
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
                if (enabled)
                {
                    m_session.setVisualizer(name);
                }
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
                Visualizer * visualizer = m_session.visualizer();
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

    // Settings

    if (ImGui::BeginTabItem("Settings"))
    {
        m_activeControlTab = ControlTab::Settings;
        renderSettingsTab();
        ImGui::EndTabItem();
    }

    // Projection

    if (ImGui::BeginTabItem("Projection"))
    {
        m_activeControlTab = ControlTab::Projection;

        if (Tools::ImGuiMonitorSelector(m_window, m_displayMonitorID)
            && m_displayWindow.isOpen())
        {
            m_displayWindow.close();
            Tools::OpenDisplayWindow(
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

bool SandboxGUI::ensureSettingsDirectory()
{
    std::error_code error;
    std::filesystem::create_directories(SettingsDirectory, error);
    if (!error)
    {
        return true;
    }

    m_settingsStatus = "Could not access the settings folder: " + error.message();
    m_settingsStatusIsError = true;
    return false;
}

void SandboxGUI::refreshSettingsFiles(const std::filesystem::path & preferred)
{
    std::filesystem::path selectedPath = preferred;
    if (selectedPath.empty()
        && m_selectedSettingsFile >= 0
        && m_selectedSettingsFile < (int)m_settingsFiles.size())
    {
        selectedPath = m_settingsFiles[m_selectedSettingsFile];
    }

    m_settingsFiles.clear();
    m_selectedSettingsFile = -1;
    if (!ensureSettingsDirectory())
    {
        return;
    }

    std::error_code error;
    std::filesystem::directory_iterator iterator(SettingsDirectory, error);
    const std::filesystem::directory_iterator end;
    while (!error && iterator != end)
    {
        std::error_code typeError;
        if (iterator->is_regular_file(typeError)
            && !typeError
            && HasJsonExtension(iterator->path()))
        {
            m_settingsFiles.push_back(iterator->path());
        }
        iterator.increment(error);
    }

    if (error)
    {
        m_settingsStatus = "Could not read the settings folder: " + error.message();
        m_settingsStatusIsError = true;
    }

    std::sort(m_settingsFiles.begin(), m_settingsFiles.end(), [](const std::filesystem::path & left, const std::filesystem::path & right)
    {
        const std::string leftName = left.filename().string();
        const std::string rightName = right.filename().string();
        const bool leftIsQuickSettings = Lowercase(leftName) == "settings.json";
        const bool rightIsQuickSettings = Lowercase(rightName) == "settings.json";
        if (leftIsQuickSettings != rightIsQuickSettings)
        {
            return leftIsQuickSettings;
        }

        const std::string lowercaseLeft = Lowercase(leftName);
        const std::string lowercaseRight = Lowercase(rightName);
        return lowercaseLeft != lowercaseRight
            ? lowercaseLeft < lowercaseRight
            : leftName < rightName;
    });

    const auto selected = std::find(m_settingsFiles.begin(), m_settingsFiles.end(), selectedPath);
    if (selected != m_settingsFiles.end())
    {
        m_selectedSettingsFile = (int)std::distance(m_settingsFiles.begin(), selected);
        return;
    }

    const auto quickSettings = std::find(m_settingsFiles.begin(), m_settingsFiles.end(), QuickSettingsFile);
    if (quickSettings != m_settingsFiles.end())
    {
        m_selectedSettingsFile = (int)std::distance(m_settingsFiles.begin(), quickSettings);
    }
    else if (!m_settingsFiles.empty())
    {
        m_selectedSettingsFile = 0;
    }
}

bool SandboxGUI::saveSettingsFile(const std::filesystem::path & filename)
{
    PROFILE_FUNCTION();
    if (!ensureSettingsDirectory())
    {
        return false;
    }
    if (!m_session.saveSettings(filename.string(), m_doubleSizeUI, m_displayMonitorID))
    {
        m_settingsStatus = "Could not save " + filename.filename().string() + ".";
        m_settingsStatusIsError = true;
        return false;
    }

    m_settingsStatus = "Saved " + filename.filename().string() + ".";
    m_settingsStatusIsError = false;
    refreshSettingsFiles(filename);
    return true;
}

bool SandboxGUI::loadSettingsFile(const std::filesystem::path & filename)
{
    PROFILE_FUNCTION();
    const std::string previousDisplayMonitorID = m_displayMonitorID;
    if (!m_session.loadSettings(filename.string(), m_doubleSizeUI, m_displayMonitorID))
    {
        m_settingsStatus = "Could not load " + filename.filename().string() + ".";
        m_settingsStatusIsError = true;
        return false;
    }

    applyUIScale();
    if (m_displayWindow.isOpen()
        && previousDisplayMonitorID != m_displayMonitorID)
    {
        m_displayWindow.close();
        Tools::OpenDisplayWindow(
            m_displayWindow,
            m_window,
            m_displayMonitorID);
    }

    m_settingsStatus = "Loaded " + filename.filename().string() + ".";
    m_settingsStatusIsError = false;
    return true;
}

void SandboxGUI::quickLoadSettings()
{
    if (ensureSettingsDirectory())
    {
        loadSettingsFile(QuickSettingsFile);
        refreshSettingsFiles(QuickSettingsFile);
    }
}

void SandboxGUI::quickSaveSettings()
{
    saveSettingsFile(QuickSettingsFile);
}

void SandboxGUI::saveNamedSettings()
{
    std::string error;
    const std::optional<std::filesystem::path> filename = BuildSettingsPath(m_settingsFilename.data(), error);
    if (!filename)
    {
        m_settingsStatus = error;
        m_settingsStatusIsError = true;
        return;
    }

    saveSettingsFile(*filename);
}

void SandboxGUI::renderSettingsTab()
{
    ImGui::TextUnformatted("Quick Settings");
    if (ImGui::Button("Quick Save Settings"))
    {
        quickSaveSettings();
    }
    ImGui::SameLine();
    if (ImGui::Button("Quick Load Settings"))
    {
        quickLoadSettings();
    }
    ImGui::TextDisabled("bin/settings/settings.json");

    ImGui::Separator();
    ImGui::TextUnformatted("Save Settings File");
    const bool enterPressed = ImGui::InputText(
        "Filename",
        m_settingsFilename.data(),
        m_settingsFilename.size(),
        ImGuiInputTextFlags_EnterReturnsTrue);
    if (ImGui::Button("Save Settings") || enterPressed)
    {
        saveNamedSettings();
    }
    ImGui::TextDisabled("Saved in bin/settings as filename.json.");

    ImGui::Separator();
    ImGui::TextUnformatted("Settings Files");
    if (ImGui::Button("Refresh##SettingsFiles"))
    {
        m_settingsStatus.clear();
        m_settingsStatusIsError = false;
        refreshSettingsFiles();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%d file%s", (int)m_settingsFiles.size(), m_settingsFiles.size() == 1 ? "" : "s");

    if (ImGui::BeginListBox(
        "##SettingsFiles",
        ImVec2(-FLT_MIN, ImGui::GetTextLineHeightWithSpacing() * 10.0f)))
    {
        if (m_settingsFiles.empty())
        {
            ImGui::TextDisabled("No settings files found.");
        }

        for (int i = 0; i < (int)m_settingsFiles.size(); i++)
        {
            const std::string name = m_settingsFiles[i].filename().string();
            const bool selected = i == m_selectedSettingsFile;
            if (ImGui::Selectable(name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick))
            {
                m_selectedSettingsFile = i;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    loadSettingsFile(m_settingsFiles[i]);
                }
            }
        }
        ImGui::EndListBox();
    }

    const bool hasSelection = m_selectedSettingsFile >= 0
        && m_selectedSettingsFile < (int)m_settingsFiles.size();
    ImGui::BeginDisabled(!hasSelection);
    if (ImGui::Button("Load Selected"))
    {
        loadSettingsFile(m_settingsFiles[m_selectedSettingsFile]);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("or double-click a file");

    if (!m_settingsStatus.empty())
    {
        if (m_settingsStatusIsError)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.30f, 1.0f), "%s", m_settingsStatus.c_str());
        }
        else
        {
            ImGui::TextWrapped("%s", m_settingsStatus.c_str());
        }
    }
}

void SandboxGUI::toggleDisplayWindow()
{
    if (!m_displayWindow.isOpen())
    {
        Tools::OpenDisplayWindow(
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
