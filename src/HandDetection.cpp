#include "HandDetection.h"
#include "Tools.h"

#include <fstream>

HandDetection::HandDetection()
{
    loadDatabase();
}
HandDetection::~HandDetection()
{
    saveDatabase();
}
void HandDetection::loadDatabase()
{
    std::cout << "Loading Dataset" << std::endl;
    std::ifstream file(m_filename);
    if (!file.good())
    {
        std::cout << "Failed to Load Dataset" << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line); // Remove labels
    while (std::getline(file, line)) // Read in data
    {
        GestureData g;
        std::stringstream s(line);
        s >> g.areaCB >> g.areaCH >> g.perimeterCH >> g.maxD >> g.minD >> g.averageD >> g.pointsCH >> g.averageA;
        for (int& slice : g.sliceCounts)
        {
            s >> slice;
        }
        s >> g.classLabel;
        m_dataset.push_back(g);
    }
}

void HandDetection::saveDatabase()
{
    std::cout << "Saving Dataset" << std::endl;
    std::ofstream file(m_filename);
    if (!file.good())
    {
        std::cout << "Couldn't write to file " << m_filename << std::endl;
        return;
    }

    // First line: labels
    const std::vector<std::string> names = { "AreaCB", "AreaCH", "PerimeterCH", "MaxD", "MinD", "AverageD", "PointsCH", "AverageA" };
    for (int i = 0; i < names.size(); ++i)
    {
        file << names[i] << " ";
    }
    for (int i = 0; i < 10; ++i)
    {
        file << "slice" << i << " ";
    }
    file << "class\n";

    // Data
    for (auto& g : m_dataset)
    {
        file << g.areaCB << " " << g.areaCH << " " << g.perimeterCH << " " << g.maxD << " " << g.minD << " " << g.averageD << " " << g.pointsCH << " " << g.averageA << " " ;
        for (int& slice : g.sliceCounts)
        {
            file << slice << " ";
        }
        file << g.classLabel << "\n";
    }
}

void HandDetection::transferCurrentData()
{
    for (auto& g : m_currentData)
    {
        m_dataset.push_back(g);
    }
}

void HandDetection::imgui()
{
    ImGui::Checkbox("Draw Hulls", &m_drawHulls);
    ImGui::Text("Dataset Size:%d", m_dataset.size());
    if (ImGui::Button("Save Dataset"))
    {
        saveDatabase();
    }
    
    ImGui::SliderFloat("Threshold", &m_thresh, 0.0, 1.0);

    if (ImGui::CollapsingHeader("Convex Hulls"))
    {
        for (size_t i = 0; i < m_hulls.size(); i++)
        {

            ImGui::Text("Hull %d: ", i);
            ImGui::SameLine();
            if (ImGui::Button(std::format("Select##{}", i).c_str()))
            {
                m_selectedHull = (int)i;
            }
            ImGui::SameLine();
            const static char* classLabels[] = {"None", "High Five", "OK", "Peace", "Rock", "Judgement"};
            ImGui::Combo(std::format("##clslabel{}", i).c_str(), &m_currentData[i].classLabel, classLabels, IM_ARRAYSIZE(classLabels));
        }
    }

    if (ImGui::Button("Add data to dataset"))
    {
        transferCurrentData();
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

    input.copyTo(m_previous, (normalized < m_thresh) & (normalized > 0.0));
    output = m_previous.clone();
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
    cv::findContours(m_segmented, m_contours, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);

    // Find Convex Hulls
    m_hulls = std::vector<std::vector<cv::Point>>(m_contours.size());

    m_currentData = std::vector<GestureData>(m_contours.size());
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

        cv::Vec2d normalizedSum;
        std::vector<double> angles (m_contours[i].size());
        auto& g = m_currentData[i];
        for (size_t j = 0; j < m_contours[i].size(); j++)
        {
            cv::Point p = m_contours[i][j];

            cv::Vec2d dif(p.x - cx, p.y - cy);
            angles[j] = atan2(dif[1], dif[0]);
            normalizedSum += cv::normalize(dif);

            double distance = sqrt(pow(cx - (double)p.x, 2) + pow(cy - (double)p.y, 2));
            g.averageD += distance;
            if (j == 0)
            {
                g.maxD = distance;
                g.minD = distance;
                continue;
            }

            if (distance > g.maxD)
            {
                g.maxD = distance;
            }

            if (distance < g.minD)
            {
                g.minD = distance;
            }
        }
        g.averageD /= (double)m_contours[i].size();
        g.averageA = atan2(normalizedSum[0], normalizedSum[1]);

        // Find slice densities
        const int slices = 10;
        double offset = CV_2PI - g.averageA;
        const double sliceSize = CV_2PI / (double)slices;
        for (double a : angles)
        {
            int s = (int)(fmod((a + offset), CV_2PI) / sliceSize);
            g.sliceCounts[s]++;
        }

        g.areaCB = contourArea / boxArea;
        g.areaCH = contourArea / hullArea;
        g.perimeterCH = contourPerimeter / hullPerimeter;
        g.pointsCH = (double)m_hulls[i].size() / (double)m_contours[i].size();

        m_classifier.classify(g);
    }
}

sf::Texture & HandDetection::getTexture()
{
    if (m_segmented.total() <= 0) { return m_texture; }
    // Ensure the input image is in the correct format (CV_32F)
    cv::Mat normalized;
    m_segmented.convertTo(normalized, CV_8U, 255.0); // Scale float [0, 1] to [0, 255]

    // Convert to RGB (SFML requires RGB format)
    cv::Mat rgb;
    cv::cvtColor(normalized, rgb, cv::COLOR_GRAY2RGBA);

    // Draw Hulls and Contours
    for (size_t i = 0; i < m_hulls.size(); i++)
    {
        cv::Scalar color = cv::Scalar(100, 100, 100, 255);
        switch (m_currentData[i].classLabel)
        {
        case 1: color = cv::Scalar(255, 0, 0, 255); break;
        case 2: color = cv::Scalar(0, 0, 255, 255); break;
        case 3: color = cv::Scalar(0, 255, 0, 255); break;
        case 4: color = cv::Scalar(255, 160, 0, 255); break;
        case 5: color = cv::Scalar(255, 0, 255, 255); break;
        }
        if (m_selectedHull == i)
        {
            if (m_drawHulls) { cv::drawContours(rgb, m_hulls, (int)i, cv::Scalar(150, 150, 150, 255), 2); }
            if (m_drawContours) { cv::drawContours(rgb, m_contours, (int)i, cv::Scalar(150, 150, 150, 255), 2); }
        }
        if (m_drawHulls) { cv::drawContours(rgb, m_hulls, (int)i, color, 1); }
        if(m_drawContours){ cv::drawContours(rgb, m_contours, (int)i, color, 1); }
    }

    // Create SFML image
    m_image.create(rgb.cols, rgb.rows, rgb.ptr());

    m_texture.loadFromImage(m_image);
    return m_texture;
}

void HandDetection::eventHandling(const sf::Event & event)
{
    if (event.type == sf::Event::JoystickButtonPressed)
    {
        switch (event.joystickButton.button)
        {
        case 0: transferCurrentData(); break; // Add to database
        case 1:
        {
            int & c = m_currentData[m_selectedHull].classLabel;
            c = (c + 1) % 6;
        }
            break; // Cycle class labels
        case 3: saveDatabase(); break; // Save
        }
    }
    else if (event.type == sf::Event::JoystickMoved)
    {
        if (event.joystickMove.axis == 6)
        {
            m_selectedHull = (m_selectedHull + ((int)event.joystickMove.position / 100) + (int)m_hulls.size()) % (int)m_hulls.size();
        }
    }
}


