#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <array>

#include "Settings.hpp"

class DataWarper
{
    cv::Mat                         m_warpMatrix;
    int                             m_dragWarpPoint = -1;
    int                             m_dragPlanarPoint = -1;
    int                             m_width = 0;
    int                             m_height = 0;
    // Perspective-transform order: top-left, top-right, bottom-left, bottom-right.
    std::array<cv::Point2f, 4>      m_warpPoints = { cv::Point2f{100, 100}, {200, 100}, {100, 200}, {200, 200} };
    std::array<cv::Point2f, 3>      m_planarPoints = { cv::Point2f{150, 0}, {75, 150}, {0, 75} };
    sf::CircleShape                 m_warpCircle;
    sf::CircleShape                 m_planarCircle;
    float                           m_dataSize = 1.0f;
    bool                            m_drawCameraRegion = true;


    // Height Adjustment
    std::array<float, 4>            m_plane = { 0.0f, 0.0f, 0.0f, 0.0f };
    cv::Mat                         m_heightOffsets;
    cv::Size                        m_planeSize;
    float                           m_baseHeight = 0.0f;
    bool                            m_updatePlane = false;
    bool                            m_planeValid = false;
    bool                            m_applyHeightAdjustment = false;

    void generateWarpMatrix();
    void generateHeightOffsets(cv::Size size);

public:

    DataWarper();
    void imgui();
    void save(Settings & save) const;
    void load(const Settings & save);
    void transformRect(const cv::Mat & input, cv::Mat & output);
    void heightAdjustment(cv::Mat & matrix);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void render(sf::RenderWindow & window);

    bool shouldAdjustHeight() const { return m_applyHeightAdjustment && (m_updatePlane || m_planeValid); }

    bool transformPoints(const std::vector<cv::Point2f> & input, std::vector<cv::Point2f> & output) const;

};
