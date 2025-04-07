#pragma once

#include "Scene.h"
#include "TopographySource.h"
#include "TopographyProcessor.h"
#include "ViewController.hpp"
#include "Save.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <opencv2/opencv.hpp>   // Include OpenCV API

class Scene_Main : public Scene
{
    cv::Mat             m_topography;

    Save                m_save;

    bool                m_drawUI = true;
    ViewController      m_viewController;
    sf::Vector2i        m_mouseScreen;
    sf::Vector2f        m_mouseWorld;
    sf::Vector2f        m_mouseDisplay;

    std::string         m_saveFile = "default.txt";

    std::string         m_sourceID = "Camera";
    std::string         m_processorID = "Colorizer";

    std::shared_ptr<TopographySource>       m_source;
    std::shared_ptr<TopographyProcessor>    m_processor;

    std::map<std::string, std::function<std::shared_ptr<TopographySource>()>> m_sourceMap;
    std::map<std::string, std::function<std::shared_ptr<TopographyProcessor>()>> m_processorMap;

    bool                m_switchWindows = false;

    void init();  
    void renderUI();
    void sUserInput();  
    void sProcessEvent(const sf::Event & event);
    void sRender();

    void setSource(const std::string & source);
    void setProcessor(const std::string & processor);

    void saveDataDump();
    
public:
    Scene_Main(GameEngine * game);

    void onFrame(float deltaTime);
    void endScene();

    inline sf::RenderWindow & mainWindow();
    inline sf::RenderWindow & displayWindow();

    template <class T>
        requires (std::is_base_of<TopographySource, T>::value)
    void registerSource(const std::string & name)
    {
        m_sourceMap.emplace(name, std::make_shared<T>);
    }

    template <class T>
        requires (std::is_base_of<TopographyProcessor, T>::value)
    void registerProcessor(const std::string & name)
    {
        m_processorMap.emplace(name, std::make_shared<T>);
    }

    void load();
    void save();
};
