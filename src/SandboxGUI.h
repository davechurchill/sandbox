#pragma once

#include "SandboxSession.hpp"
#include "ViewController.hpp"

#include <SFML/Graphics.hpp>

#include <opencv2/opencv.hpp>

#include <string>

class SandboxGUI
{
    enum class ControlTab
    {
        Source,
        Visualizer,
        Projection
    };

    sf::RenderWindow m_displayWindow;
    sf::RenderWindow m_window;
    bool m_imguiInitialized;
    SandboxSession m_session;
    sf::Clock m_deltaClock;

    cv::Mat m_projectedDepthImage;
    sf::Image m_projectedDepthSfImage;
    sf::Texture m_projectedDepthTexture;
    sf::Sprite m_projectedDepthSprite{ m_projectedDepthTexture };

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
    float m_framerate = 0.0f;

    void update();
    void renderUI();
    void sUserInput();
    void sProcessEvent(const sf::Event & event);
    void routeControlEvent(const sf::Event & event, const sf::Vector2f & mouse, bool displayWindowEvent);
    void sRender();
    void applyUIScale();
    void toggleDisplayWindow();
    void saveDataDump();
    void load();
    void save();
    void quit();

    bool isRunning() const;
    sf::RenderWindow & mainWindow();
    sf::RenderWindow & projectionWindow();

public:
    SandboxGUI();
    void run();
};
