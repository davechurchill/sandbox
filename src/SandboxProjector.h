#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <fstream>
#include <limits>

#include "Settings.hpp"

class SandBoxProjector
{
    cv::Mat                         m_projectionMatrix;
    cv::Mat                         m_terrainSource;
    cv::Mat                         m_projectedTerrain;
    cv::Mat                         m_terrainBytes;
    cv::Mat                         m_terrainRgba;
    sf::Texture                     m_terrainTexture;
    sf::Sprite                      m_terrainSprite{ m_terrainTexture };
    int                             m_dragPoint = -1;
    int                             m_dataWidth = 0;
    int                             m_dataHeight = 0;
    int                             m_finalWidth = 0;
    int                             m_finalHeight = 0;
    cv::Point2f                     m_projectionPoints[4] = { {400, 400}, {500, 400}, {400, 500}, {500, 500} };
    std::vector<sf::CircleShape>    m_projectionCircles;
    sf::Vector2f                    m_minXY;
    sf::Vector2f                    m_boxScale;
    bool                            m_drawLines = true;
    bool                            m_drawProjection = true;
    bool                            m_drawProjectedDepthMap = true;
    bool                            m_drawGrid = false;
    int                             m_gridDivisions = 8;
    int                             m_rotationQuarterTurns = 0;
    bool                            m_mirrorHorizontal = false;
    bool                            m_mirrorVertical = false;
    float                           m_handleSize = 10.0f;
    float                           m_lineColor[3] = { 1.0f, 1.0f, 1.0f };
    float                           m_lineOpacity = 1.0f;
    bool                            m_projectionValid = false;
    bool                            m_hasTerrainTexture = false;
    bool                            m_terrainTextureNeedsUpdate = true;
    std::uint64_t                   m_terrainRevision = std::numeric_limits<std::uint64_t>::max();

    void generateProjection();
    void regenerateProjection();
    void resetProjectionPoints();
    void updateProjectionHandles();
    bool ensureTerrainTexture();

public:

    SandBoxProjector();
    void imgui();
    void save(Settings & save) const;
    void load(const Settings & save);
    void project(const cv::Mat & input, cv::Mat & output);
    void setTerrain(const cv::Mat & source, std::uint64_t terrainRevision);
    bool drawTerrain(sf::RenderWindow & window, sf::Shader * shader = nullptr, bool smooth = false);
    [[nodiscard]] const sf::Texture * terrainTexture(bool smooth = false);
    bool processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    bool unprojectPoint(const sf::Vector2f & point, sf::Vector2f & dataPoint);
    void render(sf::RenderWindow & window);

    bool projectionVisible() const { return m_drawProjection; }
    bool projectedDepthMapVisible() const { return m_drawProjectedDepthMap; }
    sf::FloatRect projectionBounds() const;

    inline float getTransformedScale() const { return 1.f / m_boxScale.x; }

    inline sf::Vector2f getTransformedPosition() const { return m_minXY; }

    inline cv::Mat getProjectionMatrix()
    {
        return m_projectionMatrix;
    }
};
