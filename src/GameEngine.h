#pragma once

#include "Scene.h"
#include "Assets.h"

#include <vector>
#include <memory>
#include <SFML/Graphics.hpp>

#include "imgui.h"
#include "imgui-SFML.h"
#include "MinecraftInterface.h"

typedef std::map<std::string, std::shared_ptr<Scene>> SceneMap;

class GameEngine
{

protected:
    
    sf::RenderWindow    m_window;
    sf::Clock           m_deltaClock;
    std::string         m_currentScene;
    SceneMap            m_sceneMap;
    size_t              m_simulationSpeed = 1;
    bool                m_running = true;
    ImGuiStyle          m_originalStyle;
    MinecraftInterface  m_mcInterface;

    void update();

public:
    
    GameEngine();

    void init();
    std::shared_ptr<Scene> currentScene();

    template <class T>
    void changeScene(const std::string& sceneName, bool endCurrentScene = true)
    {
        m_window.setView(m_window.getDefaultView());
        ImGui::GetStyle() = m_originalStyle;

        if (endCurrentScene)
        {
            auto it = m_sceneMap.find(m_currentScene);
            if (it != m_sceneMap.end()) { m_sceneMap.erase(it); }
        }

        if (m_sceneMap.find(sceneName) == m_sceneMap.end())
        {
            std::shared_ptr<Scene> scene = std::make_shared<T>(this);
            m_sceneMap[sceneName] = scene;
        }

        m_currentScene = sceneName;
    }

    void quit();
    void run();
    unsigned int width() const;
    unsigned int height() const;

    sf::RenderWindow & window();
    MinecraftInterface & minecraft();
    bool isRunning();
};