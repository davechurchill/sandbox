#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>
#include <fstream>

class Calibration
{
    cv::Mat                         m_operator;
    cv::Mat                         m_boxOperator;
    int                             m_currentPoint = -1;
    int                             m_currentBoxPoint = -1;
    int                             m_dragPoint = -1;
    int                             m_dragBoxPoint = -1;
    int                             m_width = 500;
    int                             m_height = 400;
    int                             m_finalWidth = 0;
    int                             m_finalHeight = 0;
    cv::Point2f                     m_boxInteriorPoints[4] = { {100, 100}, {200, 100}, {100, 200}, {200, 200} };
    cv::Point2f                     m_boxProjectionPoints[4];
    std::vector<sf::CircleShape>    m_boxInteriorCircles;
    std::vector<sf::CircleShape>    m_boxProjectionCircles;
    int                             m_boxWidth = 500;
    int                             m_boxHeight = 400;
    sf::Vector2f                    m_minXY;
    bool                            m_applyTransform = false;
    bool                            m_applyTransform2 = false;
    bool                            m_applyAdjustment = false;
    bool                            m_drawSanboxAreaLines = true;
    bool                            m_calibrationComplete = true;
    bool                            m_calibrationBoxComplete = false;
    bool                            m_heightPointsSelected = false;
    cv::Point                       firstPoint;
    cv::Point                       secondPoint;
    cv::Point                       thirdPoint;

    void orderPoints();
    void generateWarpMatrix();

public:

    sf::Vector2f                    m_boxScale;
    Calibration();
    void imgui();
    void loadConfiguration();
    void save(std::ofstream & fout);
    void transformRect(const cv::Mat & input, cv::Mat & output);
    void transformProjection(const cv::Mat & input, cv::Mat & output);
    void heightAdjustment(cv::Mat & matrix);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void render(sf::RenderWindow & window);

    inline float getTransformedScale() const { return 1.f / m_boxScale.x; }

    inline sf::Vector2f getTransformedPosition() const { return m_minXY; }



};