#pragma once

#include "TopographySource.h"
#include "CameraFilters.hpp"
#include "DataWarper.h"

#include <opencv2/opencv.hpp>
#include <librealsense2/rs.hpp>
#include <SFML/Graphics.hpp>

enum class alignment
{
    depth,
    color,
    nothing
};

class Source_Camera : public TopographySource
{
    rs2::pipeline       m_pipe;
    bool                m_cameraConnected = false;

    DataWarper          m_warper;

    CameraFilters       m_filters;
    alignment           m_alignment = alignment::depth;
    rs2::align          m_alignment_depth = rs2::align(RS2_STREAM_DEPTH);
    rs2::align          m_alignment_color = rs2::align(RS2_STREAM_COLOR);
    bool                m_gaussianBlur = false;

    int                 m_fpsSetting = 0;

    cv::Mat             m_cvColorImage;
    sf::Image           m_sfColorImage;
    sf::Texture         m_sfColorTexture;
    sf::Sprite          m_colorSprite;

    cv::Mat             m_cvDepthImage16u;
    cv::Mat             m_cvDepthImage32f;
    cv::Mat             m_cvNormalizedDepthImage32f;
    cv::Mat             m_data;
    sf::Image           m_sfDepthImage;
    sf::Texture         m_sfDepthTexture;
    sf::Sprite          m_depthSprite;
    float               m_depthFrameUnits = 0.0f;
    float               m_maxDistance = 1.13f;
    float               m_minDistance = 0.90f;

    bool                m_drawDepth = true;
    bool                m_drawColor = false;

    void connectToCamera();
    void captureImages();

public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);

    cv::Mat getTopography();

};