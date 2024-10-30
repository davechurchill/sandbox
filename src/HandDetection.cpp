#include "HandDetection.h"
#include "Tools.h"

void HandDetection::imgui()
{
    
    m_image = Tools::matToSfImage(m_segmented);
    m_texture.loadFromImage(m_image);
    ImGui::Image(m_texture);
    
    ImGui::SliderInt("Threshold", &m_thresh, 0, 255);
    ImGui::Checkbox("Use Input", &m_useInput);
    ImGui::Checkbox("Use Previous", &m_usePrevious);
}

// Function that detects the area taken up by hands / arms and ignores it
void HandDetection::removeHands(const cv::Mat & input, cv::Mat & output, float maxDistance, float minDistance)
{
    if (m_previous.total() <= 0) // For the first frame
    {
        m_previous = input;
        output = input;
        return;
    }

    // Normalize
    cv::Mat normalized;
    normalized = 1.f - (input - minDistance) / (maxDistance - minDistance);
    cv::threshold(normalized, normalized, 0.0, 255, cv::THRESH_TOZERO);
    cv::threshold(normalized, normalized, 1.0, 255, cv::THRESH_TRUNC);

    // Binarize
    cv::Mat binarized;
    normalized.convertTo(binarized, CV_8U, 255.0);
    cv::threshold(binarized, binarized, m_thresh, 255, cv::THRESH_BINARY);
    m_segmented = binarized;
    cv::Mat mask = m_segmented == 255;
    cv::bitwise_and(input, m_previous, output, mask);
    m_previous = output;
}
