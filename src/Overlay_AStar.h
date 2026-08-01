#pragma once

#include "TopographyOverlay.hpp"

#include <cstdint>
#include <vector>

class Overlay_AStar final : public TopographyOverlay
{
    cv::Mat m_topography;
    cv::Size m_topographySize;
    cv::Point m_start;
    cv::Point m_goal;
    std::vector<cv::Point> m_path;
    std::vector<float> m_directionalSlopes;
    std::vector<std::uint8_t> m_traversableCells;
    std::vector<float> m_costFromStart;
    std::vector<float> m_estimatedCost;
    std::vector<int> m_cameFrom;
    std::vector<int> m_heapPositions;
    std::vector<int> m_openHeap;
    std::vector<bool> m_closed;

    float m_uphillPenalty = 120.0f;
    float m_downhillPenalty = 80.0f;
    float m_maximumLegalSlope = 5.0f;
    float m_pathThickness = 6.0f;
    float m_pathDistance = 0.0f;
    float m_pathCost = 0.0f;
    float m_searchTimeMilliseconds = 0.0f;
    int m_nodesExpanded = 0;
    int m_openListSize = 0;
    int m_closedListSize = 0;
    int m_movementLength = 1;
    bool m_hasStart = false;
    bool m_hasGoal = false;
    bool m_nextPointIsStart = true;
    bool m_pathFound = false;

    bool isTraversable(int x, int y) const;
    bool mapMouseToTerrain(
        const sf::Vector2f & mouse,
        TopographyProcessor & processor,
        cv::Point & terrainPoint) const;
    void precalculateSlopes();
    void calculatePath();
    void drawPath(sf::RenderWindow & window, TopographyProcessor & processor) const;
    void clearPoints();

public:
    bool usesCanvasInput() const override { return true; }
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
