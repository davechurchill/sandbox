#include "HandDetection.h"
#include "Tools.h"

void HandDetection::imgui()
{
    // Ensure the input image is in the correct format (CV_32F)
    cv::Mat normalized;
    m_segmented.convertTo(normalized, CV_8U, 255.0); // Scale float [0, 1] to [0, 255]

    // Convert to RGB (SFML requires RGB format)
    cv::Mat rgb;
    cv::cvtColor(normalized, rgb, cv::COLOR_GRAY2RGBA);

    cv::RNG rng(68);
    for (size_t i = 0; i < m_hulls.size(); i++)
    {
        cv::Scalar color = cv::Scalar(rng.uniform(0, 256), rng.uniform(0, 256), rng.uniform(0, 256), 255);
        cv::drawContours(rgb, m_hulls, (int)i, color, 3);
    }

    // Create SFML image
    m_image.create(rgb.cols, rgb.rows, rgb.ptr());

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

void HandDetection::identifyGestures(const cv::Point* area)
{
    m_gestures.clear();

    // Make mask
    cv::Mat mask = cv::Mat::ones(m_segmented.size(), CV_8U);
    cv::fillConvexPoly(mask, area, 4, cv::Scalar(0));

    m_segmented.setTo(0.0, mask);

    // Find Contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(m_segmented, contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // Find Convex Hulls
    m_hulls = std::vector<std::vector<cv::Point>>(contours.size());
    for (size_t i = 0; i < contours.size(); i++)
    {
        cv::convexHull(contours[i], m_hulls[i]);
    }
}


