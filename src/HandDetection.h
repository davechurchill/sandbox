#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include "TopographySource.h"

struct GestureData
{
    double areaCB;
    double areaCH;
    double perimeterCH;
    double maxD = 0.0;
    double minD = 0.0;
    double averageD = 0.0;
    double pointsCH;
    double averageA;
    std::array<int, 10> sliceCounts = {0,0,0,0,0, 0,0,0,0,0};
    int classLabel = 0;
};

class HandDetection
{
    int m_thresh = 218;
    int m_selectedHull = -1;
    sf::Image m_image;
    sf::Texture m_texture;

    cv::Mat m_previous;
    cv::Mat m_segmented;

    std::vector<std::vector<cv::Point>> m_hulls;
    std::vector<std::vector<cv::Point>> m_contours;

    std::vector<GestureData> m_currentData;
    std::vector<GestureData> m_dataset;

    std::string m_filename = "gestureData.txt";

    void loadDatabase();
    void saveDatabase();

    void transferCurrentData();


public:
    HandDetection();
    ~HandDetection();

    std::vector<Gesture> m_gestures;

    void imgui();
    void removeHands(const cv::Mat & input, cv::Mat & output, float maxDistance, float minDistance);
    void identifyGestures(std::vector<cv::Point> & nbox);
    sf::Texture & getTexture();
    void eventHandling(const sf::Event & event);
};