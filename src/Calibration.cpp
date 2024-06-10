#include "Calibration.h"
#include "imgui-SFML.h"
#include "imgui.h"

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
}

void Calibration::transform(cv::Mat & image)
{
    if (m_applyTransform && m_calibrationComplete)
    {
        image = m_operator * image;
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
