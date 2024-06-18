#include "Calibration.h"
#include "imgui-SFML.h"
#include "imgui.h"

#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp> 

Calibration::Calibration()
{
    float radius = 5.0;
    for (int i = 0; i < 4; ++i)
    {
        sf::CircleShape c(radius);
        c.setOrigin(radius, radius);
        c.setFillColor(sf::Color::Green);
        m_pointCircles.push_back(c);
    }
}

void Calibration::imgui()
{
    if (!m_applyTransform)
    {
        if (ImGui::Button("Select Corners"))
        {
            m_currentPoint = 0;
            m_calibrationComplete = false;
        }

        if (m_calibrationComplete && ImGui::Button("Auto Sort Corners"))
        {
            orderPoints();
        }
    }
    ImGui::InputInt("Width", &m_width);
    ImGui::InputInt("Height", &m_height);
    ImGui::Text("Width: %d", m_width);
    ImGui::Text("Height: %d", m_height);

    if (m_calibrationComplete)
    {
        ImGui::Checkbox("Apply Transform", &m_applyTransform);
    }
}

void Calibration::transform(cv::Mat & image)
{
    if (m_applyTransform && m_calibrationComplete)
    {
        cv::warpPerspective(image, image, m_operator, cv::Size(m_width, m_height));
    }
}

void Calibration::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    if (m_currentPoint > -1 && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        std::cout << mouse.x << " " << mouse.y << std::endl;
        m_points[m_currentPoint] = cv::Point(mouse.x, mouse.y);
        m_pointCircles[m_currentPoint].setPosition(mouse);
        if (++m_currentPoint > 3)
        {
            m_currentPoint = -1;
            m_calibrationComplete = true;
            generateWarpMatrix();
        }
    }
}

void Calibration::render(sf::RenderWindow & window)
{
    if (!m_applyTransform)
    {
        int n = m_calibrationComplete ? m_pointCircles.size() : m_currentPoint;
        for (int i = 0; i < n; ++i)
        {
            window.draw(m_pointCircles[i]);
        }
    }
}

void Calibration::orderPoints()
{
    int a[] = { 0, 1, 2, 3 };
    int  n = sizeof(a) / sizeof(a[0]);
    std::sort(a, a + n);

    cv::Point2f firstPoint(m_points[a[0]]);
    cv::Point2f secondPoint(m_points[a[1]]);
    cv::Point2f thirdPoint(m_points[a[2]]);
    cv::Point2f fourthPoint(m_points[a[3]]);

    while (std::next_permutation(a, a + n))
    {
        std::cout << a[0] << " " << a[1] << " " << a[2] << " " << a[3] << "\n";
        int numberOfConditionMet = 0;

        if (firstPoint.x < secondPoint.x && firstPoint.y < thirdPoint.y) { numberOfConditionMet += 1; }
        if (secondPoint.x > firstPoint.x && secondPoint.y < fourthPoint.y) { numberOfConditionMet += 1; }
        if (thirdPoint.x  < fourthPoint.x && thirdPoint.y  > firstPoint.y) { numberOfConditionMet += 1; }
        if (thirdPoint.x  < fourthPoint.x && fourthPoint.y > secondPoint.y) { numberOfConditionMet += 1; }

        if (numberOfConditionMet == 4)
        {
            break;
        }
        else
        {
            firstPoint.x = m_points[a[0]].x;
            firstPoint.y = m_points[a[0]].y;
            secondPoint.x = m_points[a[1]].x;
            secondPoint.y = m_points[a[1]].y;
            thirdPoint.x = m_points[a[2]].x;
            thirdPoint.y = m_points[a[2]].y;
            fourthPoint.x = m_points[a[3]].x;
            fourthPoint.y = m_points[a[3]].y;
        }
    }

    m_points[0] = firstPoint;
    m_points[1] = secondPoint;
    m_points[2] = thirdPoint;
    m_points[3] = fourthPoint;
}

void Calibration::generateWarpMatrix()
{
    cv::Point2f dstPoints[] = {
            cv::Point2f(0, 0),
            cv::Point2f(m_width, 0),
            cv::Point2f(0, m_height),
            cv::Point2f(m_width, m_height),
    };

    m_operator = cv::getPerspectiveTransform(m_points, dstPoints);
}