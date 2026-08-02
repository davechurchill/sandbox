#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>

#include <opencv2/core.hpp>

#include <vector>

class Terrain3DView
{
    static constexpr unsigned int WindowWidth = 1200;
    static constexpr unsigned int WindowHeight = 800;
    static constexpr int MaximumMeshResolution = 256;

    sf::RenderWindow m_window;
    std::vector<float> m_vertices;
    std::vector<float> m_normals;
    std::vector<float> m_colors;
    std::vector<float> m_textureCoordinates;
    std::vector<unsigned int> m_indices;
    sf::Texture m_visualizerTexture;
    bool m_hasVisualizerTexture = false;
    bool m_visualizationCaptured = false;

    int m_sourceColumns = 0;
    int m_sourceRows = 0;
    int m_meshColumns = 0;
    int m_meshRows = 0;
    float m_terrainWidth = 1.0f;
    float m_terrainDepth = 1.0f;
    float m_heightScale = 0.65f;

    float m_yaw = -0.75f;
    float m_pitch = 0.65f;
    float m_distance = 1.75f;
    sf::Vector3f m_target;

    sf::Vector2i m_previousMousePosition;
    bool m_orbiting = false;
    bool m_panning = false;

    void open();
    void processEvents();
    void processEvent(const sf::Event & event);
    void rebuildMesh(const cv::Mat & heightMap);
    void updateMesh(const cv::Mat & heightMap);
    void updateNormals();
    void render();
    void resetCamera();

public:
    void toggle();
    void close();
    bool isOpen() const;
    void captureVisualization(const cv::Mat & heightMap, const sf::RenderWindow & target, const cv::Mat & projectionMatrix, const sf::Vector2f & projectedPosition, float projectedScale);
    void update(const cv::Mat & heightMap);
};
