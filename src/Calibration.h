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
    int                             m_width = 500;
    int                             m_height = 400;
    int                             m_finalWidth = 0;
    int                             m_finalHeight = 0;
    cv::Point2f                     m_boxInteriorPoints[4] = { {100, 100}, {200, 100}, {100, 200}, {200, 200} };
    cv::Point2f                     m_boxProjectionPoints[4] = { {400, 400}, {500, 400}, {400, 500}, {500, 500} };
    std::vector<sf::CircleShape>    m_boxInteriorCircles;
    std::vector<sf::CircleShape>    m_boxProjectionCircles;
    int                             m_boxWidth = 500;
    int                             m_boxHeight = 400;
    sf::Vector2f                    m_minXY;
    bool                            m_applyAdjustment = false;
    bool                            m_drawSanboxAreaLines = true;
    cv::Point                       firstPoint;
    cv::Point                       secondPoint;
    cv::Point                       thirdPoint;

    void generateWarpMatrix();
    int getClickedCircleIndex(int mx, int my, std::vector<sf::CircleShape>& circles);

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