#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>
#include <fstream>

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

    void generateProjection();

public:

    SandBoxProjector();
    void imgui();
    void load(const std::string & term, std::ifstream & fin);
    void save(std::ofstream & fout);
    void project(const cv::Mat & input, cv::Mat & output);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void render(sf::RenderWindow & window);

    inline float getTransformedScale() const { return 1.f / m_boxScale.x; }

    inline sf::Vector2f getTransformedPosition() const { return m_minXY; }
};