#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Calibration
{
    cv::Mat                         m_operator;
    cv::Mat                         m_boxOperator;
    int                             m_currentPoint = -1;
    int                             m_currentBoxPoint = -1;
    int                             m_dragPoint = -1;
    int                             m_dragBoxPoint = -1;
    int                             m_width = 1200;
    int                             m_height = 780;
    int                             m_finalWidth = 0;
    int                             m_finalHeight = 0;
    cv::Point2f                     m_points[4];
    cv::Point2f                     m_boxPoints[4];
    std::vector<sf::CircleShape>    m_pointCircles;
    std::vector<sf::CircleShape>    m_pointBoxCircles;

    void orderPoints();
    void generateWarpMatrix();

public:
    int                             m_boxWidth = 500;
    int                             m_boxHeight = 400;
    float                           tempX = 0.0;
    float                           tempY = 0.0;
    bool                            m_applyTransform = false;
    bool                            m_applyTransform2 = false;
    bool                            m_applyAdjustment = false;
    bool                            m_drawSanboxAreaLines = true;
    bool                            m_calibrationComplete = false;
    bool                            m_calibrationBoxComplete = false;
    bool                            m_heightPointsSelected = false;
    cv::Point                       firstPoint;
    cv::Point                       secondPoint;
    cv::Point                       thirdPoint;

    sf::Vector2f                    m_boxScale;
    Calibration();
    void imgui();
    std::vector<cv::Point2f>  getConfig();
    std::vector<cv::Point2f>  getBoxConfig();
    std::vector<sf::CircleShape> getPointCircle();
    std::vector<sf::CircleShape> getPointBoxCircle();
    cv::Point2f getDimension();
    cv::Point2f getBoxDimension();
    void loadConfiguration();
    void transformRect(const cv::Mat & input, cv::Mat & output);
    void transformProjection(const cv::Mat & input, cv::Mat & output);
    void heightAdjustment(cv::Mat & matrix);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void render(sf::RenderWindow & window);
};