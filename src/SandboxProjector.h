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

    void generateProjection();

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
