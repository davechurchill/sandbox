#include "DataWarper.h"
#include "Profiler.hpp"
#include "imgui-SFML.h"
#include "imgui.h"
#include "Tools.h"
#include <fstream>

#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp> 

DataWarper::DataWarper()
{
    float radius = 10.0;
    sf::CircleShape circle(radius, 64);
    circle.setOrigin(radius, radius);
    circle.setFillColor(sf::Color::Green);
    
    // create the circles for the interior box selection
    m_warpCircles = std::vector<sf::CircleShape>(4, circle);

    // create the circles for height adjustment
    circle.setFillColor(sf::Color::Cyan);
    m_planarCircles = std::vector<sf::CircleShape>(3, circle);
}

void DataWarper::imgui()
{
    PROFILE_FUNCTION();
 
    ImGui::Checkbox("Draw Camera Region", &m_drawCameraRegion);

    ImGui::Checkbox("Apply Height Adjust", &m_applyHeightAdjustment);
    if (ImGui::Button("Update Height Adjustment"))
    {
        m_updatePlane = true;
    }
    ImGui::Text("Plane Norm: [%f, %f, %f]", m_plane[0], m_plane[1], m_plane[2]);

    if (ImGui::SliderFloat("Data Size", &m_dataSize, 0.1f, 1.0f))
    {
        generateWarpMatrix();
    }

    ImGui::Text("Dimensions: (%d, %d)", m_width, m_height);
}

void DataWarper::transformRect(const cv::Mat& input, cv::Mat& output)
{
    if (m_warpMatrix.rows == 0 || m_warpMatrix.cols == 0)
    {
        generateWarpMatrix();
    }
    cv::warpPerspective(input, output, m_warpMatrix, cv::Size(m_width, m_height));
}

void DataWarper::heightAdjustment(cv::Mat & matrix)
{
    PROFILE_FUNCTION();

    if (m_updatePlane)
    {
        float firstPointZ = matrix.at<float>((int)m_planarPoints[0].y, (int)m_planarPoints[0].x);
        m_baseHeight = matrix.at<float>((int)m_planarPoints[1].y, (int)m_planarPoints[1].x);
        float thirdPointZ = matrix.at<float>((int)m_planarPoints[2].y, (int)m_planarPoints[2].x);

        float vect_A[] = { m_planarPoints[1].x - m_planarPoints[0].x, m_planarPoints[1].y - m_planarPoints[0].y, m_baseHeight - firstPointZ };
        float vect_B[] = { m_planarPoints[2].x - m_planarPoints[0].x, m_planarPoints[2].y - m_planarPoints[0].y,   thirdPointZ - firstPointZ };

        m_plane[0] = vect_A[1] * vect_B[2] - vect_A[2] * vect_B[1];
        m_plane[1] = vect_A[2] * vect_B[0] - vect_A[0] * vect_B[2];
        m_plane[2] = vect_A[0] * vect_B[1] - vect_A[1] * vect_B[0];

        m_plane[3] = -(m_plane[0] * m_planarPoints[1].x + m_plane[1] * m_planarPoints[1].y + m_plane[2] * m_baseHeight);
        m_updatePlane = false;
    }

    int width = matrix.cols;
    int height = matrix.rows;
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            float newZ = (-m_plane[3] - m_plane[0] * i - m_plane[1] * j) / m_plane[2];
            matrix.at<float>(j, i) += m_baseHeight - newZ;
        }
    }
}

void DataWarper::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    PROFILE_FUNCTION();

    // detect if we have clicked a circle
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        m_dragWarpPoint = Tools::getClickedCircleIndex(mouse.x, mouse.y, m_warpCircles);
        m_dragPlanarPoint = Tools::getClickedCircleIndex(mouse.x, mouse.y, m_planarCircles);
    }

    // if we have released the mouse button
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
    {
        m_dragWarpPoint = -1;
        m_dragPlanarPoint = -1;
    }

    // if the mouse moved and we are dragging something, update its position and regenerate the matrix
    if (event.type == sf::Event::MouseMoved)
    {
        if (m_dragWarpPoint != -1) 
        {
            m_warpPoints[m_dragWarpPoint] = cv::Point((int)mouse.x, (int)mouse.y);
            m_warpCircles[m_dragWarpPoint].setPosition(mouse);
            generateWarpMatrix();
        }

        if (m_dragPlanarPoint != -1)
        {
            m_planarPoints[m_dragPlanarPoint] = cv::Point((int)mouse.x, (int)mouse.y);
            m_planarCircles[m_dragPlanarPoint].setPosition(mouse);
        }
    }
}

void DataWarper::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (m_drawCameraRegion)
    {
        // draw the circles and lines used to calibrate the interior of the sandbox for the depth camera
        for (size_t i = 0; i < m_warpCircles.size(); ++i)
        {
            m_warpCircles[i].setPosition({ m_warpPoints[i].x, m_warpPoints[i].y });
            window.draw(m_warpCircles[i]);
        }

        sf::VertexArray boxInteriorVertices(sf::LinesStrip);
        boxInteriorVertices.append(sf::Vertex(m_warpCircles[0].getPosition()));
        boxInteriorVertices.append(sf::Vertex(m_warpCircles[1].getPosition()));
        boxInteriorVertices.append(sf::Vertex(m_warpCircles[3].getPosition()));
        boxInteriorVertices.append(sf::Vertex(m_warpCircles[2].getPosition()));
        boxInteriorVertices.append(sf::Vertex(m_warpCircles[0].getPosition()));
        window.draw(boxInteriorVertices);
    }

    if (m_applyHeightAdjustment)
    {
        // draw the circles and lines used to calibrate the height adjustment
        for (size_t i = 0; i < m_planarCircles.size(); ++i)
        {
            m_planarCircles[i].setPosition({ m_planarPoints[i].x, m_planarPoints[i].y });
            window.draw(m_planarCircles[i]);
        }

        sf::VertexArray planarVertices(sf::LinesStrip);
        planarVertices.append(sf::Vertex(m_planarCircles[0].getPosition()));
        planarVertices.append(sf::Vertex(m_planarCircles[1].getPosition()));
        planarVertices.append(sf::Vertex(m_planarCircles[2].getPosition()));
        planarVertices.append(sf::Vertex(m_planarCircles[0].getPosition()));
        window.draw(planarVertices);
    }
}

void DataWarper::generateWarpMatrix()
{
    PROFILE_FUNCTION();

    cv::Point2f line1 = m_warpPoints[0] - m_warpPoints[1];
    cv::Point2f line2 = m_warpPoints[0] - m_warpPoints[2];
    float w = sqrtf(line1.dot(line1));
    float h = sqrtf(line2.dot(line2));
    m_width = (int)(w * m_dataSize);
    m_height = (int)(h * m_dataSize);

    cv::Point2f dstPoints[] = {
            cv::Point2f(0, 0),
            cv::Point2f((float)m_width, 0),
            cv::Point2f(0, (float)m_height),
            cv::Point2f((float)m_width, (float)m_height),
    };
    m_warpMatrix = cv::getPerspectiveTransform(m_warpPoints, dstPoints);
}

void DataWarper::save(Save & save) const
{
    std::copy(std::cbegin(m_warpPoints), std::cend(m_warpPoints), std::begin(save.warpPoints));
    save.applyHeightAdjustment = m_applyHeightAdjustment;
    std::copy(std::cbegin(m_planarPoints), std::cend(m_planarPoints), std::begin(save.planarPoints));
}
void DataWarper::load(const Save & save)
{
    std::copy(std::cbegin(save.warpPoints), std::cend(save.warpPoints), std::begin(m_warpPoints));
    m_applyHeightAdjustment = save.applyHeightAdjustment;
    std::copy(std::cbegin(save.planarPoints), std::cend(save.planarPoints), std::begin(m_planarPoints));
}