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
    if (ImGui::Button("Select Corners"))
    {
        m_currentPoint = 0;
        m_calibrationComplete = false;
    }
    if (m_calibrationComplete)
    {
        ImGui::Checkbox("Apply Transform", &m_applyTransform);
    }
    ImGui::InputInt("Width", &m_width);
    ImGui::InputInt("Height", &m_height);
    ImGui::Text("Width: %d", m_width);
    ImGui::Text("Height: %d", m_height);

    ImGui::Image(m_texture, sf::Vector2f(m_width / 4, m_height / 4));
}

void Calibration::transform(cv::Mat & image)
{
    if (m_applyTransform && m_calibrationComplete)
    {
        //image = m_operator * image;
        cv::Point2f srcPoints [] = {
            cv::Point(m_points[0].x, m_points[0].y),
            cv::Point(m_points[1].x, m_points[1].y),
            cv::Point(m_points[2].x, m_points[2].y),
            cv::Point(m_points[3].x, m_points[3].y)
        };

        cv::Point2f dstPoints[] = {
            cv::Point(0, 0),
            cv::Point(m_width, 0),
            cv::Point(0, m_height),
            cv::Point(m_width, m_height),
        };

        cv::Mat output;
        cv::Mat Matrix = cv::getPerspectiveTransform(srcPoints, dstPoints);
        cv::warpPerspective(image, output, Matrix, cv::Size(m_width, m_height));

        sf::Image i;
        i.create(output.cols, output.rows, output.ptr());
        m_texture.loadFromImage(i);
    }
}

void Calibration::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    if (m_currentPoint > -1 && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        std::cout << mouse.x << " " << mouse.y << std::endl;
        m_points[m_currentPoint] = mouse;
        m_pointCircles[m_currentPoint].setPosition(mouse);
        if (++m_currentPoint > 3)
        {
            m_currentPoint = -1;
            m_calibrationComplete = true;
        }
    }
}

void Calibration::render(sf::RenderWindow & window)
{
    int n = m_calibrationComplete ? m_pointCircles.size() : m_currentPoint;
    for (int i = 0; i < n; ++i)
    {
        window.draw(m_pointCircles[i]);
    }
}
