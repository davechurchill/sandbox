#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>
#include <fstream>

class Calibration
{
    cv::Mat                         m_operator;
    cv::Mat                         m_boxOperator;
    int                             m_dragPoint = -1;
    int                             m_dragBoxPoint = -1;
    int                             m_dragPlanarPoint = -1;
    int                             m_width = 1280;
    int                             m_height = 720;
    int                             m_finalWidth = 0;
    int                             m_finalHeight = 0;
    cv::Point2f                     m_boxInteriorPoints[4] = { {100, 100}, {200, 100}, {100, 200}, {200, 200} };
    cv::Point2f                     m_boxProjectionPoints[4] = { {400, 400}, {500, 400}, {400, 500}, {500, 500} };
    cv::Point2f                     m_planarPoints[3] = { {150, 0}, {75, 150}, {0, 75} };
    std::vector<sf::CircleShape>    m_boxInteriorCircles;
    std::vector<sf::CircleShape>    m_boxProjectionCircles;
    std::vector<sf::CircleShape>    m_planarCircles;
    int                             m_boxWidth = 1280;
    int                             m_boxHeight = 720;
    sf::Vector2f                    m_minXY;
    bool                            m_drawSanboxAreaLines = true;
    sf::Vector2f                    m_boxScale;

    // Height Adjustment
    float                           m_plane[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float                           m_baseHeight;
    bool                            m_updatePlane = false;
    bool                            m_applyHeightAdjustment = false;

    void generateWarpMatrix();
    int getClickedCircleIndex(float mx, float my, std::vector<sf::CircleShape>& circles);

public:

    Calibration();
    void imgui();
    void loadConfiguration();
    void save(std::ofstream & fout);
    void transformRect(const cv::Mat & input, cv::Mat & output);
    void transformProjection(const cv::Mat & input, cv::Mat & output);
    void heightAdjustment(cv::Mat & matrix);
    void processDebugEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void processDisplayEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void render(sf::RenderWindow & window, sf::RenderWindow & displayWindow);

    inline float getTransformedScale() const { return 1.f / m_boxScale.x; }

    inline sf::Vector2f getTransformedPosition() const { return m_minXY; }

    inline bool shouldAdjustHeight() const { return m_applyHeightAdjustment; }



};