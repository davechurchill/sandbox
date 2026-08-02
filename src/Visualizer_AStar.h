#pragma once

#include "Visualizer.hpp"

#include <cstdint>
#include <vector>

class Visualizer_AStar final : public Visualizer
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
    bool mapMouseToTerrain(const sf::Vector2f & mouse, cv::Point & terrainPoint) const;
    void precalculateSlopes();
    void calculatePath();
    void drawPath(sf::RenderWindow & window) const;
    void clearPoints();

public:
    static constexpr std::string_view Name = "Pathfinding (A*)";
    explicit Visualizer_AStar(SandboxProjector & projector) : Visualizer(Name, projector) {}

    bool usesCanvasInput() const override { return true; }
    void init() override;
    void imgui() override;
    void process(const TerrainFrame & data) override;
    void render(sf::RenderWindow & window) override;
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse) override;
    void save(Settings & save) const override;
    void load(const Settings & save) override;
};
