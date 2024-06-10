#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Calibration
{
    cv::Mat                         m_operator;
    int                             m_currentPoint = -1;
    bool                            m_calibrationComplete = false;
    bool                            m_applyTransform = false;
    int                             m_width = 1200;
    int                             m_height = 780;
    sf::Vector2f                    m_points[4];
    std::vector<sf::CircleShape>    m_pointCircles;

    sf::Texture                     m_texture;

public:
    Calibration();
    void imgui();
    void transform(cv::Mat & image);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void render(sf::RenderWindow & window);
};