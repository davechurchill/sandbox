#pragma once

#include "SandboxSession.h"
#include "Terrain3DView.h"
#include "ViewController.hpp"

#include <SFML/Graphics.hpp>

#include <opencv2/opencv.hpp>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

class SandboxGUI
{
    enum class ControlTab
    {
        Source,
        Visualizer,
        Settings,
        Projection
    };

    sf::RenderWindow m_displayWindow;
    sf::RenderWindow m_window;
    bool m_imguiInitialized;
    SandboxSession m_session;
    Terrain3DView m_terrain3DView;
    sf::Clock m_deltaClock;

    bool m_running = true;
    bool m_drawUI = true;
    bool m_doubleSizeUI = true;
    ControlTab m_activeControlTab = ControlTab::Source;
    ViewController m_viewController;
    sf::Vector2i m_mouseScreen;
    sf::Vector2f m_mouseWorld;
    sf::Vector2f m_mouseDisplay;
    std::string m_displayMonitorID;
    bool m_switchWindows = false;
    bool m_initialViewPending = true;
    float m_framerate = 0.0f;

    std::vector<std::filesystem::path> m_settingsFiles;
    std::array<char, 128> m_settingsFilename{};
    std::string m_settingsStatus;
    int m_selectedSettingsFile = -1;
    bool m_settingsStatusIsError = false;

    void update();
    void renderUI();
    void sUserInput();
    void sProcessEvent(const sf::Event & event);
    void routeControlEvent(const sf::Event & event, const sf::Vector2f & mouse, bool displayWindowEvent);
    void sRender();
    void frameInitialView();
    void applyUIScale();
    void toggleDisplayWindow();
    void saveDataDump();
    bool ensureSettingsDirectory();
    void refreshSettingsFiles(const std::filesystem::path & preferred = {});
    bool saveSettingsFile(const std::filesystem::path & filename);
    bool loadSettingsFile(const std::filesystem::path & filename);
    void quickLoadSettings();
    void quickSaveSettings();
    void saveNamedSettings();
    void renderSettingsTab();
    void quit();

    bool isRunning() const;
    sf::RenderWindow & mainWindow();
    sf::RenderWindow & projectionWindow();

public:
    SandboxGUI();
    void run();
};
