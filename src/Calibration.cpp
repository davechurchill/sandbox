#include "Calibration.h"
#include "imgui-SFML.h"
#include "imgui.h"
#include <fstream>

#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp> 

Calibration::Calibration()
{
    float radius = 10.0;
    sf::CircleShape circle(radius, 64);
    circle.setOrigin(radius, radius);
    circle.setFillColor(sf::Color::Green);
    
    // create the circles for the interior box selection
    m_boxInteriorCircles = std::vector<sf::CircleShape>(4, circle);

    // create the circles for the display correction
    circle.setFillColor(sf::Color::Magenta);
    m_boxProjectionCircles = std::vector<sf::CircleShape>(4, circle);
}

void Calibration::imgui()
{
    ImGui::Checkbox("Apply Height Adjustment", &m_applyAdjustment);
 
    ImGui::Checkbox("Sandbox Lines", &m_drawSanboxAreaLines);
}

void Calibration::transformRect(const cv::Mat& input, cv::Mat& output)
{
    cv::warpPerspective(input, output, m_operator, cv::Size(m_width, m_height));
}

void Calibration::transformProjection(const cv::Mat & input, cv::Mat & output)
{
    cv::warpPerspective(input, output, m_boxOperator, cv::Size(m_finalWidth, m_finalHeight));
}

void Calibration::heightAdjustment(cv::Mat & matrix)
{
    int width = matrix.cols;
    int height = matrix.rows;

    /*float topLeft = matrix.at<float>(0, 0);

    int centerX = width / 2;
    int centerY = height / 2;

    float centerValue = matrix.at<float>(centerY, centerX);

    float bottomRight = matrix.at<float>(0, width - 1);

    float vect_A[] = { centerX, centerY, centerValue - topLeft };
    float vect_B[] = { width - 1, 0, bottomRight - topLeft };
    float cross_P[] = { 0.0, 0.0, 0.0 };*/

    float firstPointZ  = matrix.at<float>(firstPoint.y,  firstPoint.x);
    float secondPointZ = matrix.at<float>(secondPoint.y, secondPoint.x);
    float thirdPointZ  = matrix.at<float>(thirdPoint.y,  thirdPoint.x);

    float vect_A[] = { (float)(secondPoint.x - firstPoint.x), (float)(secondPoint.y - firstPoint.y), secondPointZ - firstPointZ };
    float vect_B[] = { (float)(thirdPoint.x - firstPoint.x), (float)(thirdPoint.y - firstPoint.y),   thirdPointZ - firstPointZ };
    float cross_P[] = { 0.0, 0.0, 0.0 };

    cross_P[0] = vect_A[1] * vect_B[2] - vect_A[2] * vect_B[1];
    cross_P[1] = vect_A[2] * vect_B[0] - vect_A[0] * vect_B[2];
    cross_P[2] = vect_A[0] * vect_B[1] - vect_A[1] * vect_B[0];

    //plane equation
    float d = -(cross_P[0] * secondPoint.x + cross_P[1] * secondPoint.y + cross_P[2] * secondPointZ);
    for (int i = 0; i < width; i++)
    {
        for (int j = 0; j < height; j++)
        {
            float newZ = (-d - cross_P[0] * i - cross_P[1] * j) / cross_P[2];
            matrix.at<float>(j, i) += secondPointZ - newZ;
        }
    }
}

// given an (mx, my) mouse position, return the index of the first circle the contains the position
// returns -1 if the mouse position is not inside any circle
int Calibration::getClickedCircleIndex(float mx, float my, std::vector<sf::CircleShape>& circles)
{
    for (int i = 0; i < circles.size(); i++)
    {
        float dx = mx - circles[i].getPosition().x;
        float dy = my - circles[i].getPosition().y;
        float d2 = dx * dx + dy * dy;
        float rad2 = circles[i].getRadius() * circles[i].getRadius();
        if (d2 <= rad2) { return i; }
    }

    return -1;
}

void Calibration::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    // detect if we have clicked a circle
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        m_dragPoint = getClickedCircleIndex(mouse.x, mouse.y, m_boxInteriorCircles);
        m_dragBoxPoint = getClickedCircleIndex(mouse.x, mouse.y, m_boxProjectionCircles);
    }

    // if we have released the mouse button
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
    {
        m_dragPoint = -1;
        m_dragBoxPoint = -1;
    }

    // if the mouse moved and we are dragging something, update its position and regenerate the matrix
    if (event.type == sf::Event::MouseMoved)
    {
        if (m_dragPoint != -1) 
        {
            m_boxInteriorPoints[m_dragPoint] = cv::Point((int)mouse.x, (int)mouse.y);
            m_boxInteriorCircles[m_dragPoint].setPosition(mouse);
            generateWarpMatrix();
        }

        if (m_dragBoxPoint != -1) 
        {
            m_boxProjectionPoints[m_dragBoxPoint] = cv::Point((int)mouse.x, (int)mouse.y);
            m_boxProjectionCircles[m_dragBoxPoint].setPosition(mouse);
            generateWarpMatrix();
        }
    }
}

void Calibration::render(sf::RenderWindow & window)
{
    // draw the circles and lines used to calibrate the interior of the sandbox for the depth camera
    for (size_t i = 0; i < m_boxInteriorCircles.size(); ++i)
    {
        m_boxInteriorCircles[i].setPosition({ m_boxInteriorPoints[i].x, m_boxInteriorPoints[i].y });
        window.draw(m_boxInteriorCircles[i]);
    }

    sf::VertexArray boxInteriorVertices(sf::LinesStrip);
    boxInteriorVertices.append(sf::Vertex(m_boxInteriorCircles[0].getPosition()));
    boxInteriorVertices.append(sf::Vertex(m_boxInteriorCircles[1].getPosition()));
    boxInteriorVertices.append(sf::Vertex(m_boxInteriorCircles[3].getPosition()));
    boxInteriorVertices.append(sf::Vertex(m_boxInteriorCircles[2].getPosition()));
    boxInteriorVertices.append(sf::Vertex(m_boxInteriorCircles[0].getPosition()));
    window.draw(boxInteriorVertices);

    // draw the circles outlining where the box should be 
    if (m_drawSanboxAreaLines)
    {
        for (size_t i = 0; i < m_boxProjectionCircles.size(); ++i)
        {
            window.draw(m_boxProjectionCircles[i]);
        }

        sf::VertexArray boxProjectionVertices(sf::LinesStrip);
        boxProjectionVertices.append(sf::Vertex(m_boxProjectionCircles[0].getPosition()));
        boxProjectionVertices.append(sf::Vertex(m_boxProjectionCircles[1].getPosition()));
        boxProjectionVertices.append(sf::Vertex(m_boxProjectionCircles[3].getPosition()));
        boxProjectionVertices.append(sf::Vertex(m_boxProjectionCircles[2].getPosition()));
        boxProjectionVertices.append(sf::Vertex(m_boxProjectionCircles[0].getPosition()));
        window.draw(boxProjectionVertices);
    }
}

void Calibration::generateWarpMatrix()
{
    cv::Point2f dstPoints[] = {
            cv::Point2f(0, 0),
            cv::Point2f((float)m_width, 0),
            cv::Point2f(0, (float)m_height),
            cv::Point2f((float)m_width, (float)m_height),
    };
    m_operator = cv::getPerspectiveTransform(m_boxInteriorPoints, dstPoints);

    cv::Point2f boxPoints[] = { m_boxProjectionPoints[0], m_boxProjectionPoints[1], m_boxProjectionPoints[2], m_boxProjectionPoints[3] };

    m_minXY.x = boxPoints[0].x;
    m_minXY.y = boxPoints[0].y;
    float maxX = boxPoints[0].x;
    float maxY = boxPoints[0].y;
    for (int i = 0; i < 4; i++)
    {
        if (boxPoints[i].x < m_minXY.x)
        {
            m_minXY.x = boxPoints[i].x;
        }
        if (boxPoints[i].x > maxX)
        {
            maxX = boxPoints[i].x;
        }
        if (boxPoints[i].y < m_minXY.y)
        {
            m_minXY.y = boxPoints[i].y;
        }
        if (boxPoints[i].y > maxY)
        {
            maxY = boxPoints[i].y;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        boxPoints[i].x -= m_minXY.x;
        boxPoints[i].y -= m_minXY.y;
    }
    m_boxWidth = (int)(maxX - m_minXY.x);
    m_boxHeight = (int)(maxY - m_minXY.y);

    float ratio = (float)m_boxHeight / m_boxWidth;
    m_finalWidth = (int)(m_width * 1.5f);
    m_finalHeight = (int)(m_finalWidth * ratio);
    m_boxScale = sf::Vector2f((float)m_finalWidth / m_boxWidth, (float)m_finalHeight / m_boxHeight);

    for (int i = 0; i < 4; i++)
    {
        boxPoints[i].x *= m_boxScale.x;
        boxPoints[i].y *= m_boxScale.y;
    }

    m_boxOperator = cv::getPerspectiveTransform(dstPoints, boxPoints);
  
}

void Calibration::loadConfiguration()
{
    std::ifstream fin("config.txt");
    std::string temp;
    float x, y;
    while (fin >> temp)
    {
        if (temp == "m_points[0]")              { fin >> m_boxInteriorPoints[0].x >> m_boxInteriorPoints[0].y; }
        else if (temp == "m_boxPoints[0]")      { fin >> m_boxProjectionPoints[0].x >> m_boxProjectionPoints[0].y; }
        else if (temp == "m_points[1]")         { fin >> m_boxInteriorPoints[1].x >> m_boxInteriorPoints[1].y; }
        else if (temp == "m_boxPoints[1]")      { fin >> m_boxProjectionPoints[1].x >> m_boxProjectionPoints[1].y; }
        else if (temp == "m_points[2]")         { fin >> m_boxInteriorPoints[2].x >> m_boxInteriorPoints[2].y; }
        else if (temp == "m_boxPoints[2]")      { fin >> m_boxProjectionPoints[2].x >> m_boxProjectionPoints[2].y; }
        else if (temp == "m_points[3]")         { fin >> m_boxInteriorPoints[3].x >> m_boxInteriorPoints[3].y; }
        else if (temp == "m_boxPoints[3]")      { fin >> m_boxProjectionPoints[3].x >> m_boxProjectionPoints[3].y; }
        else if (temp == "m_width")             { fin >> m_width; }
        else if (temp == "m_boxWidth")          { fin >> m_boxWidth; }
        else if (temp == "m_height")            { fin >> m_height; }
        else if (temp == "m_boxHeight")         { fin >> m_boxHeight; }
        else if (temp == "m_pointCircles[0]")   { fin >> x >> y; m_boxInteriorCircles[0].setPosition(x, y); }
        else if (temp == "m_pointCircles[1]")   { fin >> x >> y; m_boxInteriorCircles[1].setPosition(x, y); }
        else if (temp == "m_pointCircles[2]")   { fin >> x >> y; m_boxInteriorCircles[2].setPosition(x, y); }
        else if (temp == "m_pointCircles[3]")   { fin >> x >> y; m_boxInteriorCircles[3].setPosition(x, y); }
        else if (temp == "m_pointBoxCircles[0]") { fin >> x >> y; m_boxProjectionCircles[0].setPosition(x, y); }
        else if (temp == "m_pointBoxCircles[1]") { fin >> x >> y; m_boxProjectionCircles[1].setPosition(x, y); }
        else if (temp == "m_pointBoxCircles[2]") { fin >> x >> y; m_boxProjectionCircles[2].setPosition(x, y); }
        else if (temp == "m_pointBoxCircles[3]") { fin >> x >> y; m_boxProjectionCircles[3].setPosition(x, y); }
        else if (temp == "m_drawSanboxAreaLines") { fin >> m_drawSanboxAreaLines; }
    }

    generateWarpMatrix();
}

void Calibration::save(std::ofstream & fout)
{
    fout << "m_drawSanboxAreaLines" << " " << m_drawSanboxAreaLines << "\n";

    fout << "m_points[0]" << " " << m_boxInteriorPoints[0].x << " " << m_boxInteriorPoints[0].y << "\n";
    fout << "m_points[1]" << " " << m_boxInteriorPoints[1].x << " " << m_boxInteriorPoints[1].y << "\n";
    fout << "m_points[2]" << " " << m_boxInteriorPoints[2].x << " " << m_boxInteriorPoints[2].y << "\n";
    fout << "m_points[3]" << " " << m_boxInteriorPoints[3].x << " " << m_boxInteriorPoints[3].y << "\n";

    fout << "m_pointCircles[0]" << " " << m_boxInteriorCircles[0].getPosition().x << " " << m_boxInteriorCircles[0].getPosition().y << "\n";
    fout << "m_pointCircles[1]" << " " << m_boxInteriorCircles[1].getPosition().x << " " << m_boxInteriorCircles[1].getPosition().y << "\n";
    fout << "m_pointCircles[2]" << " " << m_boxInteriorCircles[2].getPosition().x << " " << m_boxInteriorCircles[2].getPosition().y << "\n";
    fout << "m_pointCircles[3]" << " " << m_boxInteriorCircles[3].getPosition().x << " " << m_boxInteriorCircles[3].getPosition().y << "\n";

    fout << "m_boxPoints[0]" << " " << m_boxProjectionPoints[0].x << " " << m_boxProjectionPoints[0].y << "\n";
    fout << "m_boxPoints[1]" << " " << m_boxProjectionPoints[1].x << " " << m_boxProjectionPoints[1].y << "\n";
    fout << "m_boxPoints[2]" << " " << m_boxProjectionPoints[2].x << " " << m_boxProjectionPoints[2].y << "\n";
    fout << "m_boxPoints[3]" << " " << m_boxProjectionPoints[3].x << " " << m_boxProjectionPoints[3].y << "\n";

    fout << "m_pointBoxCircles[0]" << " " << m_boxProjectionCircles[0].getPosition().x << " " << m_boxProjectionCircles[0].getPosition().y << "\n";
    fout << "m_pointBoxCircles[1]" << " " << m_boxProjectionCircles[1].getPosition().x << " " << m_boxProjectionCircles[1].getPosition().y << "\n";
    fout << "m_pointBoxCircles[2]" << " " << m_boxProjectionCircles[2].getPosition().x << " " << m_boxProjectionCircles[2].getPosition().y << "\n";
    fout << "m_pointBoxCircles[3]" << " " << m_boxProjectionCircles[3].getPosition().x << " " << m_boxProjectionCircles[3].getPosition().y << "\n";

    fout << "m_width" << " " << m_width << "\n";
    fout << "m_height" << " " << m_height << "\n";

    fout << "m_boxWidth" << " " << m_boxWidth << "\n";
    fout << "m_boxHeight" << " " << m_boxHeight << "\n";
}