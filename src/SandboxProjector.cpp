#include "SandBoxProjector.h"
#include "Profiler.hpp"
#include "imgui-SFML.h"
#include "imgui.h"
#include "Tools.h"

#include <fstream>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp> 

SandBoxProjector::SandBoxProjector()
{
    float radius = 10.0;
    sf::CircleShape circle(radius, 64);
    circle.setOrigin(radius, radius);
    circle.setFillColor(sf::Color::Green);

    // create the circles for the display correction
    circle.setFillColor(sf::Color::Magenta);
    m_projectionCircles = std::vector<sf::CircleShape>(4, circle);
}

void SandBoxProjector::project(const cv::Mat & input, cv::Mat & output)
{
    // Check to see if data matrix has changed in size and generate the projection matrix again if so
    int width = input.cols;
    int height = input.rows;
    if (width != m_dataWidth || height != m_dataHeight || m_projectionMatrix.rows == 0 || m_projectionMatrix.cols == 0)
    {
        m_dataWidth = width;
        m_dataHeight = height;
        generateProjection();
    }

    // Apply projection
    cv::warpPerspective(input, output, m_projectionMatrix, cv::Size(m_finalWidth, m_finalHeight));
}

void SandBoxProjector::imgui()
{
    PROFILE_FUNCTION();

    ImGui::Checkbox("Show Projection", &m_drawProjection);
    ImGui::Checkbox("Show Projection Lines", &m_drawLines);
}

bool SandBoxProjector::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    PROFILE_FUNCTION();

    // detect if we have clicked a circle
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        m_dragPoint = Tools::getClickedCircleIndex(mouse.x, mouse.y, m_projectionCircles);
    }

    // if we have released the mouse button
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
    {
        m_dragPoint = -1;
    }

    // if the mouse moved and we are dragging something, update its position and regenerate the matrix
    if (event.type == sf::Event::MouseMoved)
    {
        if (m_dragPoint != -1)
        {
            m_projectionPoints[m_dragPoint] = cv::Point((int)mouse.x, (int)mouse.y);
            m_projectionCircles[m_dragPoint].setPosition(mouse);
            generateProjection();
        }
    }

    return m_dragPoint != -1;
}

void SandBoxProjector::render(sf::RenderWindow & window)
{
    if (!m_drawProjection) { return; }
    PROFILE_FUNCTION();
    if (m_drawLines)
    {
        for (size_t i = 0; i < m_projectionCircles.size(); ++i)
        {
            m_projectionCircles[i].setPosition(m_projectionPoints[i].x, m_projectionPoints[i].y);
            window.draw(m_projectionCircles[i]);
        }

        sf::VertexArray projectionVertices(sf::LinesStrip);
        projectionVertices.append(sf::Vertex(m_projectionCircles[0].getPosition()));
        projectionVertices.append(sf::Vertex(m_projectionCircles[1].getPosition()));
        projectionVertices.append(sf::Vertex(m_projectionCircles[3].getPosition()));
        projectionVertices.append(sf::Vertex(m_projectionCircles[2].getPosition()));
        projectionVertices.append(sf::Vertex(m_projectionCircles[0].getPosition()));
        window.draw(projectionVertices);
    }
}

void SandBoxProjector::generateProjection()
{
    PROFILE_FUNCTION();

    cv::Point2f dataCorners[] = {
            cv::Point2f(0, 0),
            cv::Point2f((float)m_dataWidth, 0),
            cv::Point2f(0, (float)m_dataHeight),
            cv::Point2f((float)m_dataWidth, (float)m_dataHeight),
    };

    cv::Point2f boxPoints[] = { m_projectionPoints[0], m_projectionPoints[1], m_projectionPoints[2], m_projectionPoints[3] };

    m_minXY.x = boxPoints[0].x;
    m_minXY.y = boxPoints[0].y;
    float maxX = boxPoints[0].x;
    float maxY = boxPoints[0].y;
    for (int i = 0; i < 4; i++)
    {
        if (boxPoints[i].x < m_minXY.x) { m_minXY.x = boxPoints[i].x; }
        if (boxPoints[i].x > maxX)      { maxX = boxPoints[i].x; }
        if (boxPoints[i].y < m_minXY.y) { m_minXY.y = boxPoints[i].y; }
        if (boxPoints[i].y > maxY)      { maxY = boxPoints[i].y; }
    }

    for (int i = 0; i < 4; i++)
    {
        boxPoints[i].x -= m_minXY.x;
        boxPoints[i].y -= m_minXY.y;
    }
    int boxWidth = (int)(maxX - m_minXY.x);
    int boxHeight = (int)(maxY - m_minXY.y);

    float ratio = (float)boxHeight / boxWidth;
    m_finalWidth = (int)(m_dataWidth * 1.5f);
    m_finalHeight = (int)(m_finalWidth * ratio);
    m_boxScale = sf::Vector2f((float)m_finalWidth / boxWidth, (float)m_finalHeight / boxHeight);

    for (int i = 0; i < 4; i++)
    {
        boxPoints[i].x *= m_boxScale.x;
        boxPoints[i].y *= m_boxScale.y;
    }

    m_projectionMatrix = cv::getPerspectiveTransform(dataCorners, boxPoints);
}

void SandBoxProjector::save(Save& save) const
{
    std::copy(std::cbegin(m_projectionPoints), std::cend(m_projectionPoints), std::begin(save.projectionPoints));
    save.drawLines = m_drawLines;
    save.drawProjection = m_drawProjection;
}

void SandBoxProjector::load(const Save& save)
{
    std::copy(std::cbegin(save.projectionPoints), std::cend(save.projectionPoints), std::begin(m_projectionPoints));
    m_drawLines = save.drawLines;
    m_drawProjection = save.drawProjection;
}
