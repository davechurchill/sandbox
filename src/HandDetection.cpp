#include "HandDetection.h"

void HandDetection::imgui(const cv::Mat & input)
{
    try
    {
        cv::Mat adjusted;
        input.convertTo(adjusted, CV_8U, 255.0);
        adjusted = 255 - adjusted;

        cv::Canny(adjusted, adjusted, m_thresh, m_thresh * 2);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(adjusted, contours, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);

        std::vector<std::vector<cv::Point>> hull(contours.size());
        //ImGui::Text("Contours: %d", contours.size());
        for (size_t i = 0; i < contours.size(); i++)
        {
            cv::convexHull(contours[i], hull[i]);
            //ImGui::Text("   C%d: %d", i, contours[i].size());
        }
        cv::Mat drawing = cv::Mat::zeros(adjusted.size(), CV_8UC4) + cv::Scalar(0,0,0,255);
        for (size_t i = 0; i < contours.size(); i++)
        {
            cv::Scalar color = cv::Scalar(255, 0, 0, 255);
            //cv::drawContours(drawing, contours, (int)i, color, -1);
            cv::drawContours(drawing, hull, (int)i, color, -1);
        }

        //cv::connectedComponents(drawing, drawing);

        image.create(drawing.cols, drawing.rows, drawing.ptr());
        tex.loadFromImage(image);
        ImGui::Image(tex, sf::Color::White);
    }
    catch (cv::Exception e)
    {
        ImGui::Text("Error: %s", e.msg);
    }
    ImGui::SliderInt("Threshold", &m_thresh, 10, 255);
}
