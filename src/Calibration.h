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
    bool                            m_calibrationComplete = false;
    bool                            m_calibrationBoxComplete = false;
    bool                            m_applyTransform = false;
    bool                            m_applyTransform2 = false;
    int                             m_width = 1200;
    int                             m_height = 780;
    cv::Point2f                     m_points[4];
    cv::Point2f                     m_boxPoints[4];
    std::vector<sf::CircleShape>    m_pointCircles;
    std::vector<sf::CircleShape>    m_pointBoxCircles;

    void orderPoints();
    void generateWarpMatrix();

public:
    int                             m_boxWidth = 1200;
    int                             m_boxHeight = 780;
    float                           tempX = 0.0;
    float                           tempY = 0.0;
    Calibration();
    void imgui();
    std::vector<cv::Point2f>  getConfig();
    cv::Point2f getDimension();
    void loadConfiguration();
    void transform(cv::Mat & input, cv::Mat& output);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void render(sf::RenderWindow & window);
};