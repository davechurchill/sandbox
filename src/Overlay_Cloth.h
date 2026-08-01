#pragma once

#include "TopographyOverlay.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <vector>

class Overlay_Cloth : public TopographyOverlay
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
    TopographyProcessor *   m_processor = nullptr;
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
    bool mapMouseToTerrain(
        const sf::Vector2f & mouse,
        TopographyProcessor & processor,
        cv::Point2f & terrainPosition) const;
    void renderCloth(sf::RenderWindow & window, TopographyProcessor & processor) const;

public:
    void initOverlay() override;
    void imguiOverlay() override;
    void processTopographyOverlay(
        const IntermediateData & data,
        TopographyProcessor & processor) override;
    void renderOverlay(
        sf::RenderWindow & window,
        TopographyProcessor & processor) override;
    void processOverlayEvent(
        const sf::Event & event,
        const sf::Vector2f & mouse,
        TopographyProcessor & processor) override;
    void saveOverlay(Settings & save) const override;
    void loadOverlay(const Settings & save) override;
};
