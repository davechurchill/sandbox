#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>
#include <fstream>

#include "Save.hpp"

class DataWarper
{
    cv::Mat                         m_warpMatrix;
    int                             m_dragWarpPoint = -1;
    int                             m_dragPlanarPoint = -1;
    int                             m_width = 1280;
    int                             m_height = 720;
    cv::Point2f                     m_warpPoints[4] = { {100, 100}, {200, 100}, {100, 200}, {200, 200} };
    cv::Point2f                     m_planarPoints[3] = { {150, 0}, {75, 150}, {0, 75} };
    std::vector<sf::CircleShape>    m_warpCircles;
    std::vector<sf::CircleShape>    m_planarCircles;
    float                           m_dataSize = 1.0f;
    bool                            m_drawCameraRegion = true;


    // Height Adjustment
    float                           m_plane[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    float                           m_baseHeight;
    bool                            m_updatePlane = false;
    bool                            m_applyHeightAdjustment = false;

    void generateWarpMatrix();

public:

    DataWarper();
    void imgui();
    void save(Save & save) const;
    void load(const Save & save);
    void transformRect(const cv::Mat & input, cv::Mat & output);
    void heightAdjustment(cv::Mat & matrix);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void render(sf::RenderWindow & window);

    inline bool shouldAdjustHeight() const { return m_applyHeightAdjustment; }



};