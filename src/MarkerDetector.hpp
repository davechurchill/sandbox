#pragma once

#include <opencv2/objdetect/aruco_detector.hpp>
#include <opencv2/opencv.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include "MarkerData.h"

#include <numeric>

class MarkerDetector
{
    cv::aruco::Dictionary m_dictonary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
    cv::aruco::ArucoDetector m_detector = cv::aruco::ArucoDetector(m_dictonary, cv::aruco::DetectorParameters());
    int m_printID = 0;
public:
    std::vector<int> markerIds;
    std::vector<std::vector<cv::Point2f>> markerCorners;

    void saveMarkerImage(int id) const
    {
        if (id > 249)
        {
            std::cout << "Error: ArUco id must be lower than 250" << std::endl;
            return;
        }
        cv::Mat image;
        cv::aruco::generateImageMarker(m_dictonary, id, 200, image, 1);
        cv::imwrite(std::format("marker{}.png", id), image);
    }

    void identifyMarkers(const cv::Mat & image)
    {
        markerCorners.clear();
        markerIds.clear();
        m_detector.detectMarkers(image, markerCorners, markerIds);
        cv::aruco::drawDetectedMarkers(image, markerCorners, markerIds);
    }

    void drawMarkers(cv::Mat& image)
    {
        cv::aruco::drawDetectedMarkers(image, markerCorners, markerIds);
    }

    inline void imgui()
    {
        ImGui::InputInt("Marker ID", &m_printID);
        if (ImGui::Button("Save Marker Image"))
        {
            saveMarkerImage(m_printID);
        }
    }
};