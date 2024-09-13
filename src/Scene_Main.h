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

    int                 m_sourceID = TopographySource::Camera;
    int                 m_processorID = TopographyProcessor::Colorizer;

    std::shared_ptr<TopographySource>       m_source;
    std::shared_ptr<TopographyProcessor>    m_processor;

    void init();  
    void renderUI();
    void sUserInput();  
    void sProcessEvent(const sf::Event & event);
    void sRender();

    void load();
    void save();

    void setSource(int source);
    void setProcessor(int processor);

    void saveDataDump();
    
public:

    Scene_Main(GameEngine * game);

    void onFrame();
    void endScene();
};
