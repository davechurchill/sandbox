#pragma once

#include "TopographyVisualizer.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <vector>

class Visualizer_Cloth : public TopographyVisualizer
{
    struct Vector3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct Node
    {
        Vector3 position;
        Vector3 velocity;
    };

    struct Spring
    {
        int first = 0;
        int second = 0;
        float restLength = 0.0f;
        float stiffnessScale = 1.0f;
    };

    cv::Mat                 m_topography;
    cv::Size                m_topographySize;
    std::vector<Node>       m_nodes;
    std::vector<Spring>     m_springs;

    cv::Point2f m_center;
    int   m_cellsX = 13;
    int   m_cellsY = 9;
    float m_clothSize = 150.0f;
    float m_sheetTransparency = 0.85f;
    float m_stiffness = 24.0f;
    float m_damping = 2.2f;
    float m_gravity = 2.0f;
    float m_windX = 0.0f;
    float m_windY = 0.0f;
    bool  m_centerSet = false;
    bool  m_resetPending = true;

    static float vectorLength(const Vector3 & vector);
    int nodeIndex(int column, int row) const;
    float sampleHeight(const cv::Point2f & position) const;
    float heightScale() const;
    void addSpring(int first, int second, float stiffnessScale);
    void createCloth();
    void updateCloth(float deltaTime);
    bool mapMouseToTerrain(const sf::Vector2f & mouse, cv::Point2f & terrainPosition) const;
    void renderCloth(sf::RenderWindow & window) const;

public:
    static constexpr std::string_view Name = "Cloth Sheet";
    Visualizer_Cloth() : TopographyVisualizer(Name) {}

    bool usesCanvasInput() const override { return true; }
    void init() override;
    void imgui() override;
    void process(const TerrainFrame & data) override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
