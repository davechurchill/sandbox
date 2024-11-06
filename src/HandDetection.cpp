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

    for (size_t i = 0; i < m_hulls.size(); i++)
    {
        cv::Scalar color = cv::Scalar(240, 0, 0, 255);
        if (m_selectedHull == i)
        {
            color = cv::Scalar(0, 250, 0, 255);
        }
        cv::drawContours(rgb, m_hulls, (int)i, color, 2);
    }

    // Create SFML image
    m_image.create(rgb.cols, rgb.rows, rgb.ptr());

    for (size_t i = 0; i < m_hulls.size(); i++)
    {
        auto m = cv::moments(m_hulls[i]);
        int cx = (int)(m.m10 / m.m00);
        int cy = (int)(m.m01 / m.m00);
        m_image.setPixel(cx, cy, sf::Color(0, 0, 255, 255));
    }

    m_texture.loadFromImage(m_image);
    ImGui::Image(m_texture);
    
    ImGui::SliderInt("Threshold", &m_thresh, 0, 255);

    if (ImGui::CollapsingHeader("Convex Hulls"))
    {
        for (size_t i = 0; i < m_hulls.size(); i++)
        {

            ImGui::Text("Hull %d: ", i);
            ImGui::SameLine();
            if (ImGui::Button(std::format("Select##{}", i).c_str()))
            {
                m_selectedHull = i;
            }
        }
    }
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

void HandDetection::identifyGestures(std::vector<cv::Point> & box)
{
    m_gestures.clear();
    if (m_segmented.total() <= 0)
    {
        return;
    }

    double boxArea = cv::contourArea(box, true);

    // Make mask
    cv::Mat mask = cv::Mat::ones(m_segmented.size(), CV_8U);
    cv::fillConvexPoly(mask, box, cv::Scalar(0));

    m_segmented.setTo(0.0, mask);

    m_contours = std::vector<std::vector<cv::Point>>();
    // Find Contours
    cv::findContours(m_segmented, m_contours, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

    // Find Convex Hulls
    m_hulls = std::vector<std::vector<cv::Point>>(m_contours.size());
    for (size_t i = 0; i < m_contours.size(); i++)
    {
        cv::convexHull(m_contours[i], m_hulls[i]);
        auto m = cv::moments(m_hulls[i]);
        int cx = (int)(m.m10 / m.m00);
        int cy = (int)(m.m01 / m.m00);

        double hullArea = cv::contourArea(m_hulls[i], true);
        double hullPerimeter = cv::arcLength(m_hulls[i], true);
        double contourArea = cv::contourArea(m_contours[i], true);
        double contourPerimeter = cv::arcLength(m_contours[i], true);

        double max = 0.0;
        double min = 0.0;
        for (size_t j = 0; j < m_contours[i].size(); j++)
        {
            cv::Point p = m_contours[i][j];
            double distance = sqrt(pow(cx - (double)p.x, 2) + pow(cy - (double)p.y, 2));
            if (j == 0)
            {
                max = distance;
                min = distance;
                continue;
            }

            if (distance > max)
            {
                max = distance;
            }

            if (distance < min)
            {
                min = distance;
            }
        }

        std::vector<double> x = {
            contourArea / boxArea,
            contourArea / hullArea,
            contourPerimeter / hullPerimeter,
            max,
            min
        };
    }
}


