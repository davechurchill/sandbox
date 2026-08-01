#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>
#include <fstream>

#include "Save.hpp"

class SandBoxProjector
{
    cv::Mat                         m_projectionMatrix;
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
    bool                            m_drawGrid = false;
    int                             m_gridDivisions = 8;
    int                             m_rotationQuarterTurns = 0;
    bool                            m_mirrorHorizontal = false;
    bool                            m_mirrorVertical = false;
    float                           m_handleSize = 10.0f;
    float                           m_lineColor[3] = { 1.0f, 1.0f, 1.0f };
    float                           m_lineOpacity = 1.0f;
    bool                            m_projectionValid = false;

    void generateProjection();
    void resetProjectionPoints();
    void updateProjectionHandles();

public:

    SandBoxProjector();
    void imgui();
    void save(Save & save) const;
    void load(const Save & save);
    void project(const cv::Mat & input, cv::Mat & output);
    bool processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void render(sf::RenderWindow & window);

    inline float getTransformedScale() const { return 1.f / m_boxScale.x; }

    inline sf::Vector2f getTransformedPosition() const { return m_minXY; }

    inline cv::Mat getProjectionMatrix()
    {
        generateProjection();
        return m_projectionMatrix;
    }
};
