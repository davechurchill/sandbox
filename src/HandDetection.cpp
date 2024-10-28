#include "HandDetection.h"

void HandDetection::imgui(const cv::Mat & input)
{
    try
    {
        cv::Mat adjusted;
        input.convertTo(adjusted, CV_8U, 255.0);
        //adjusted = 255 - adjusted;

        cv::Canny(adjusted, adjusted, m_thresh, m_thresh * 2);

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(adjusted, contours, cv::RETR_TREE, cv::ContourApproximationModes::CHAIN_APPROX_TC89_KCOS);

        std::vector<std::vector<cv::Point>> hull(contours.size());
        //ImGui::Text("Contours: %d", contours.size());
        for (size_t i = 0; i < contours.size(); i++)
        {
            cv::convexHull(contours[i], hull[i],true);
            //ImGui::Text("   C%d: %d", i, contours[i].size());
        }
        cv::Mat drawing = cv::Mat::zeros(adjusted.size(), CV_8UC4) + cv::Scalar(0,0,0,255);
        for (size_t i = 0; i < contours.size(); i++)
        {
            cv::drawContours(drawing, hull, (int)i, cv::Scalar(0, 255, 0, 255), -1);
            cv::drawContours(drawing, contours, (int)i, cv::Scalar(255, 0, 0, 255), -1);
        }
        /*cv::Mat components = cv::Mat::zeros(adjusted.size(), CV_16U);
        int labels = cv::connectedComponents(adjusted, components);
        ImGui::SliderInt("Label", &label, 0, labels);
        if (label >= labels) 
        {
            label = 0;
        }
        cv::Mat drawing = cv::Mat::zeros(adjusted.size(), CV_8UC4) + cv::Scalar(0,0,0,255);
        drawing.setTo(cv::Scalar(255, 255, 255, 255), components == label);*/

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
