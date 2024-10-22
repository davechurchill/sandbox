#include "HandDetection.h"

#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"

void HandDetection::imgui(const cv::Mat & input)
{
    try
    {
        cv::Mat adjusted;
        input.convertTo(adjusted, CV_8U, 255.0);
        adjusted = 255 - adjusted;

        cv::Canny(adjusted, adjusted, m_thresh, m_thresh * 2);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(adjusted, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

        std::vector<std::vector<cv::Point> >hull(contours.size());
        for (size_t i = 0; i < contours.size(); i++)
        {
            cv::convexHull(contours[i], hull[i]);
        }

        ImGui::Text("Contours: %d", contours.size());

        cv::RNG rng(68);
        cv::Mat drawing = cv::Mat::zeros(adjusted.size(), CV_8UC3);
        for (size_t i = 0; i < contours.size(); i++)
        {
            cv::Scalar color = cv::Scalar(rng.uniform(0, 256), rng.uniform(0, 256), rng.uniform(0, 256));
            cv::drawContours(drawing, contours, (int)i, color);
            cv::drawContours(drawing, hull, (int)i, color);
        }
        cv::cvtColor(drawing, drawing, cv::COLOR_BGR2RGBA);

        sf::Image image;
        image.create(drawing.cols, drawing.rows, drawing.ptr());
        sf::Texture tex;
        tex.loadFromImage(image);
        ImGui::Image(tex);
    }
    catch (cv::Exception e)
    {
        ImGui::Text("Error: %s", e.msg);
    }
    ImGui::SliderInt("Threshold", &m_thresh, 10, 255);
}
