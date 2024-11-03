#include "HandDetection.h"
#include "Tools.h"

void HandDetection::imgui()
{
    m_image = Tools::matToSfImage(m_segmented);
    m_texture.loadFromImage(m_image);
    ImGui::Image(m_texture);
    
    ImGui::SliderInt("Threshold", &m_thresh, 0, 255);
}

// Function that detects the area taken up by hands / arms and ignores it
void HandDetection::removeHands(const cv::Mat & input, cv::Mat & output, float maxDistance, float minDistance)
{
    if (m_previous.total() <= 0) // For the first frame
    {
        m_previous = input.clone();
        output = input;
        return;
    }
    // Normalize
    cv::Mat normalized;
    normalized = 1.f - (input - minDistance) / (maxDistance - minDistance);

    // Binarize
    cv::Mat binarized;
    normalized.convertTo(binarized, CV_8U, 255.0);
    cv::threshold(binarized, m_segmented, m_thresh, 255, cv::THRESH_BINARY);
    cv::Mat mask = m_segmented == 255;
    cv::Mat in = input.clone();
    cv::Mat prev = m_previous.clone();
    in.setTo(0.0, mask);
    prev.setTo(0.0, 1 - mask);
    output = in + prev;
    m_previous = output.clone();
}
