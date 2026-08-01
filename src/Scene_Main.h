#pragma once

#include "SandboxSession.hpp"
#include "Scene.h"
#include "ViewController.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <opencv2/opencv.hpp>   // Include OpenCV API

#include <string>

class Scene_Main : public Scene
{
    enum class ControlTab
    {
        Source,
        Processor,
        Projection,
        Overlay
    };

    SandboxSession      m_session;
    cv::Mat             m_projectedDepthImage;
    sf::Image           m_projectedDepthSfImage;
    sf::Texture         m_projectedDepthTexture;
    sf::Sprite          m_projectedDepthSprite{ m_projectedDepthTexture };

    bool                m_drawUI = true;
    bool                m_doubleSizeUI = true;
    ControlTab          m_activeControlTab = ControlTab::Source;
    ViewController      m_viewController;
    sf::Vector2i        m_mouseScreen;
    sf::Vector2f        m_mouseWorld;
    sf::Vector2f        m_mouseDisplay;
    std::string         m_displayMonitorID;

    bool                m_switchWindows = false;

    void init();  
    void renderUI();
    void sUserInput();  
    void sProcessEvent(const sf::Event & event);
    void sRender();
    void applyUIScale();

    void toggleDisplayWindow();
    void openDisplayWindow();

    void saveDataDump();
    
public:
    Scene_Main(GameEngine * game);

    void onFrame(float deltaTime);
    void endScene();

    inline sf::RenderWindow & mainWindow();
    inline sf::RenderWindow & displayWindow();

    void load();
    void save();
};
