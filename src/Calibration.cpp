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
    float radius = 5.0;
    for (int i = 0; i < 4; ++i)
    {
        sf::CircleShape c(radius);
        c.setOrigin(radius, radius);
        c.setFillColor(sf::Color::Green);
        c.setPosition(-3247, -3247);
        m_boxInteriorCircles.push_back(c);
    }

    float radius2 = 5.0;
    for (int i = 0; i < 4; ++i)
    {
        sf::CircleShape c(radius2);
        c.setOrigin(radius2, radius2);
        c.setFillColor(sf::Color::Magenta);
        c.setPosition(-3247, -3247);
        m_boxProjectionCircles.push_back(c);
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

            for (int i = 0; i < 4; ++i)
            {
               m_boxInteriorCircles[i].setPosition(-3247, -3247);
            }
        }
        if (m_calibrationComplete)
        {

            ImGui::SameLine();
            if (ImGui::Button("Auto Sort Corners"))
            {
                orderPoints();
                generateWarpMatrix();
            }
        }

        if (ImGui::Button("Select Box Corners"))
        {
            m_currentBoxPoint = 0;
            m_calibrationBoxComplete = false;

            for (int i = 0; i < 4; ++i)
            {
                m_boxProjectionCircles[i].setPosition(-3247, -3247);
            }
        }

        if (ImGui::Button("Select Height Points"))
        {
            m_heightPointsSelected = true;

            firstPoint.x = -3247;
            secondPoint.x = -3247;
            thirdPoint.x = -3247;
        }

        bool w = ImGui::InputInt("Width", &m_width);
        bool h = ImGui::InputInt("Height", &m_height);
        if (w || h)
        {
            generateWarpMatrix();
        }
    }

    if (m_calibrationComplete)
    {
        ImGui::Checkbox("Apply Transform", &m_applyTransform);
        ImGui::Checkbox("Apply Transform2", &m_applyTransform2);
        ImGui::Checkbox("Apply Height Adjustment", &m_applyAdjustment);
    }

    if (m_calibrationBoxComplete)
    {
        ImGui::Checkbox("Sandbox Lines", &m_drawSanboxAreaLines);
    }
}

void Calibration::transformRect(const cv::Mat& input, cv::Mat& output)
{
    if (m_applyTransform && m_calibrationComplete)
    {
        cv::warpPerspective(input, output, m_operator, cv::Size(m_width, m_height));
    }
}

void Calibration::transformProjection(const cv::Mat & input, cv::Mat & output)
{
    if (m_applyTransform2 && m_calibrationBoxComplete)
    {
        cv::warpPerspective(input, output, m_boxOperator, cv::Size(m_finalWidth, m_finalHeight));
    }
}

void Calibration::heightAdjustment(cv::Mat & matrix)
{
    if (m_applyTransform && m_calibrationComplete && m_applyAdjustment)
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

        float firstPointZ = matrix.at<float>(firstPoint.y, firstPoint.x);
        float secondPointZ = matrix.at<float>(secondPoint.y, secondPoint.x);
        float thirdPointZ = matrix.at<float>(thirdPoint.y, thirdPoint.x);

        float vect_A[] = { secondPoint.x - firstPoint.x, secondPoint.y - firstPoint.y, secondPointZ - firstPointZ };
        float vect_B[] = { thirdPoint.x - firstPoint.x, thirdPoint.y - firstPoint.y,   thirdPointZ - firstPointZ };
        float cross_P[] = { 0.0, 0.0, 0.0 };

        cross_P[0] = vect_A[1] * vect_B[2] - vect_A[2] * vect_B[1];
        cross_P[1] = vect_A[2] * vect_B[0] - vect_A[0] * vect_B[2];
        cross_P[2] = vect_A[0] * vect_B[1] - vect_A[1] * vect_B[0];

        //plane equation
        float d = -(cross_P[0] * secondPoint.x + cross_P[1] * secondPoint.y + cross_P[2] * secondPointZ);
        for (size_t i = 0; i < width; i++)
        {
            for (size_t j = 0; j < height; j++)
            {
                float newZ = (-d - cross_P[0] * i - cross_P[1] * j) / cross_P[2];
                matrix.at<float>(j, i) += secondPointZ - newZ;
            }
        }
    }
}

void Calibration::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    if (m_dragPoint != -1) {
        m_boxInteriorPoints[m_dragPoint] = cv::Point(mouse.x, mouse.y);
        m_boxInteriorCircles[m_dragPoint].setPosition(mouse);
        generateWarpMatrix();
    }

    if (m_dragBoxPoint != -1) {
        m_boxProjectionPoints[m_dragBoxPoint] = cv::Point(mouse.x, mouse.y);
        m_boxProjectionCircles[m_dragBoxPoint].setPosition(mouse);
        generateWarpMatrix();
    }

    if (m_calibrationComplete == true && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        int i = 0;
        while (i < 4)
        {
            float xDistance = mouse.x - m_boxInteriorCircles[i].getPosition().x;
            float yDistance = mouse.y - m_boxInteriorCircles[i].getPosition().y;
            float pointsDistance = sqrt((xDistance * xDistance) + (yDistance * yDistance));

            if (pointsDistance <= m_boxInteriorCircles[i].getRadius())
            {
                m_dragPoint = i;
            }
            i++;
        }
    }

    if (m_calibrationBoxComplete == true && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        int i = 0;
        while (i < 4)
        {
            float xDistance = mouse.x - m_boxProjectionCircles[i].getPosition().x;
            float yDistance = mouse.y - m_boxProjectionCircles[i].getPosition().y;
            float pointsDistance = sqrt((xDistance * xDistance) + (yDistance * yDistance));

            if (pointsDistance <= m_boxProjectionCircles[i].getRadius())
            {
                m_dragBoxPoint = i;
            }
            i++;
        }
    }

    if (m_currentPoint > -1 && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        std::cout << mouse.x << " " << mouse.y << std::endl;
        m_boxInteriorPoints[m_currentPoint] = cv::Point(mouse.x, mouse.y);
        m_boxInteriorCircles[m_currentPoint].setPosition(mouse);
        if (++m_currentPoint > 3)
        {
            m_currentPoint = -1;
            m_calibrationComplete = true;
            generateWarpMatrix();
        }
    }

    if (m_currentBoxPoint > -1 && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        std::cout << mouse.x << " " << mouse.y << std::endl;
        m_boxProjectionPoints[m_currentBoxPoint] = cv::Point(mouse.x, mouse.y);
        m_boxProjectionCircles[m_currentBoxPoint].setPosition(mouse);
        if (++m_currentBoxPoint > 3)
        {
            m_currentBoxPoint = -1;
            m_calibrationBoxComplete = true;
            generateWarpMatrix();
        }
    }
    if (m_heightPointsSelected == true && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        std::cout << mouse.x << " " << mouse.y << std::endl;

        if (firstPoint.x == -3247)
        {
            firstPoint.x = mouse.x;
            firstPoint.y = mouse.y;
        }

        else if (secondPoint.x == -3247)
        {
            secondPoint.x = mouse.x;
            secondPoint.y = mouse.y;
        }

        else 
        {
            thirdPoint.x = mouse.x;
            thirdPoint.y = mouse.y;
            m_heightPointsSelected = false;

            // Print the coordinates of each point
            std::cout << "First Point: x = " << firstPoint.x << ", y = " << firstPoint.y << std::endl;
            std::cout << "Second Point: x = " << secondPoint.x << ", y = " << secondPoint.y << std::endl;
            std::cout << "Third Point: x = " << thirdPoint.x << ", y = " << thirdPoint.y << std::endl;
        }
    }

    if (m_dragPoint != -1 && event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
    {
        m_dragPoint = -1;
        generateWarpMatrix();
    }

    if (m_dragBoxPoint != -1 && event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
    {
        m_dragBoxPoint = -1;
        generateWarpMatrix();
    }

}

void Calibration::render(sf::RenderWindow & window)
{
    //if (!m_applyTransform)
    {
        for (size_t i = 0; i < m_boxInteriorCircles.size(); ++i)
        {
            m_boxInteriorCircles[i].setPosition({ m_boxInteriorPoints[i].x, m_boxInteriorPoints[i].y });
            window.draw(m_boxInteriorCircles[i]);
        }

        int nBox = m_calibrationBoxComplete ? m_boxProjectionCircles.size() : m_currentBoxPoint;
        for (int i = 0; i < nBox; ++i)
        {
            if (m_drawSanboxAreaLines)
            {
                window.draw(m_boxProjectionCircles[i]);
            }
        }
    }

    if (m_boxInteriorCircles[1].getPosition().x != -3247)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_boxInteriorCircles[0].getPosition().x, m_boxInteriorCircles[0].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_boxInteriorCircles[1].getPosition().x, m_boxInteriorCircles[1].getPosition().y))
        };
        window.draw(line, 2, sf::Lines);
    }

    if (m_boxProjectionCircles[1].getPosition().x != -3247  && m_drawSanboxAreaLines)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_boxProjectionCircles[0].getPosition().x, m_boxProjectionCircles[0].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_boxProjectionCircles[1].getPosition().x, m_boxProjectionCircles[1].getPosition().y))
        };
        window.draw(line, 2, sf::Lines);
    }

    if (m_boxInteriorCircles[2].getPosition().x != -3247)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_boxInteriorCircles[0].getPosition().x, m_boxInteriorCircles[0].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_boxInteriorCircles[2].getPosition().x, m_boxInteriorCircles[2].getPosition().y))
        };
        window.draw(line, 2, sf::Lines);
    }

    if (m_boxProjectionCircles[2].getPosition().x != -3247 && m_drawSanboxAreaLines)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_boxProjectionCircles[0].getPosition().x, m_boxProjectionCircles[0].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_boxProjectionCircles[2].getPosition().x, m_boxProjectionCircles[2].getPosition().y))
        };
        window.draw(line, 2, sf::Lines);
    }

    if (m_calibrationComplete )
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_boxInteriorCircles[1].getPosition().x, m_boxInteriorCircles[1].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_boxInteriorCircles[3].getPosition().x, m_boxInteriorCircles[3].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_boxInteriorCircles[2].getPosition().x, m_boxInteriorCircles[2].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_boxInteriorCircles[3].getPosition().x, m_boxInteriorCircles[3].getPosition().y))
        };
        window.draw(line, 4, sf::Lines);
    }

    if (m_calibrationBoxComplete && m_drawSanboxAreaLines)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_boxProjectionCircles[1].getPosition().x, m_boxProjectionCircles[1].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_boxProjectionCircles[3].getPosition().x, m_boxProjectionCircles[3].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_boxProjectionCircles[2].getPosition().x, m_boxProjectionCircles[2].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_boxProjectionCircles[3].getPosition().x, m_boxProjectionCircles[3].getPosition().y))
        };
        window.draw(line, 4, sf::Lines);
    }
}


void Calibration::orderPoints()
{
    int a[] = { 0, 1, 2, 3 };
    int  n = sizeof(a) / sizeof(a[0]);
    std::sort(a, a + n);

    cv::Point2f firstPoint(m_boxInteriorPoints[a[0]]);
    cv::Point2f secondPoint(m_boxInteriorPoints[a[1]]);
    cv::Point2f thirdPoint(m_boxInteriorPoints[a[2]]);
    cv::Point2f fourthPoint(m_boxInteriorPoints[a[3]]);

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
            firstPoint.x = m_boxInteriorPoints[a[0]].x;
            firstPoint.y = m_boxInteriorPoints[a[0]].y;
            secondPoint.x = m_boxInteriorPoints[a[1]].x;
            secondPoint.y = m_boxInteriorPoints[a[1]].y;
            thirdPoint.x = m_boxInteriorPoints[a[2]].x;
            thirdPoint.y = m_boxInteriorPoints[a[2]].y;
            fourthPoint.x = m_boxInteriorPoints[a[3]].x;
            fourthPoint.y = m_boxInteriorPoints[a[3]].y;
        }
    }

    m_boxInteriorPoints[0] = firstPoint;
    m_boxInteriorPoints[1] = secondPoint;
    m_boxInteriorPoints[2] = thirdPoint;
    m_boxInteriorPoints[3] = fourthPoint;
}

void Calibration::generateWarpMatrix()
{
    cv::Point2f dstPoints[] = {
            cv::Point2f(0, 0),
            cv::Point2f(m_width, 0),
            cv::Point2f(0, m_height),
            cv::Point2f(m_width, m_height),
    };
    m_operator = cv::getPerspectiveTransform(m_boxInteriorPoints, dstPoints);

    if (m_calibrationBoxComplete)
    {
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
        m_boxWidth = maxX - m_minXY.x;
        m_boxHeight = maxY - m_minXY.y;

        float ratio = (float)m_boxHeight / m_boxWidth;
        m_finalWidth = m_width * 1.5f;
        m_finalHeight = m_finalWidth * ratio;
        m_boxScale = sf::Vector2f((float)m_finalWidth / m_boxWidth, (float)m_finalHeight / m_boxHeight);

        for (int i = 0; i < 4; i++)
        {
            boxPoints[i].x *= m_boxScale.x;
            boxPoints[i].y *= m_boxScale.y;
        }

        m_boxOperator = cv::getPerspectiveTransform(dstPoints, boxPoints);
    }
}

void Calibration::loadConfiguration()
{
    std::ifstream fin("config.txt");
    std::string temp;
    while (fin >> temp)
    {
        if (temp == "m_points[0]")
        {
            fin >> m_boxInteriorPoints[0].x;
            fin >> m_boxInteriorPoints[0].y;
        }

        if (temp == "m_boxPoints[0]")
        {
            fin >> m_boxProjectionPoints[0].x;
            fin >> m_boxProjectionPoints[0].y;
        }

        if (temp == "m_points[1]")
        {
            fin >> m_boxInteriorPoints[1].x;
            fin >> m_boxInteriorPoints[1].y;
        }

        if (temp == "m_boxPoints[1]")
        {
            fin >> m_boxProjectionPoints[1].x;
            fin >> m_boxProjectionPoints[1].y;
        }

        if (temp == "m_points[2]")
        {
            fin >> m_boxInteriorPoints[2].x;
            fin >> m_boxInteriorPoints[2].y;
        }

        if (temp == "m_boxPoints[2]")
        {
            fin >> m_boxProjectionPoints[2].x;
            fin >> m_boxProjectionPoints[2].y;
        }

        if (temp == "m_points[3]")
        {
            fin >> m_boxInteriorPoints[3].x;
            fin >> m_boxInteriorPoints[3].y;
        }

        if (temp == "m_boxPoints[3]")
        {
            fin >> m_boxProjectionPoints[3].x;
            fin >> m_boxProjectionPoints[3].y;
        }

        if (temp == "m_width")
        {
            fin >> m_width;
        }

        if (temp == "m_boxWidth")
        {
            fin >> m_boxWidth;
        }

        if (temp == "m_height")
        {
            fin >> m_height;
        }

        if (temp == "m_boxHeight")
        {
            fin >> m_boxHeight;
        }

        if (temp == "m_pointCircles[0]")
        {
            float x, y;
            fin >> x >> y;
            m_boxInteriorCircles[0].setPosition(x, y);
        }

        if (temp == "m_pointCircles[1]")
        {
            float x, y;
            fin >> x >> y;
            m_boxInteriorCircles[1].setPosition(x, y);
        }

        if (temp == "m_pointCircles[2]")
        {
            float x, y;
            fin >> x >> y;
            m_boxInteriorCircles[2].setPosition(x, y);
        }

        if (temp == "m_pointCircles[3]")
        {
            float x, y;
            fin >> x >> y;
            m_boxInteriorCircles[3].setPosition(x, y);
        }

        if (temp == "m_pointBoxCircles[0]")
        {
            float x, y;
            fin >> x >> y;
            m_boxProjectionCircles[0].setPosition(x, y);
        }

        if (temp == "m_pointBoxCircles[1]")
        {
            float x, y;
            fin >> x >> y;
            m_boxProjectionCircles[1].setPosition(x, y);
        }

        if (temp == "m_pointBoxCircles[2]")
        {
            float x, y;
            fin >> x >> y;
            m_boxProjectionCircles[2].setPosition(x, y);
        }

        if (temp == "m_pointBoxCircles[3]")
        {
            float x, y;
            fin >> x >> y;
            m_boxProjectionCircles[3].setPosition(x, y);
        }

        if (temp == "m_applyTransform")
        {
            fin >> m_applyTransform;
        }

        if (temp == "m_applyTransform2")
        {
            fin >> m_applyTransform2;
        }

        if (temp == "m_drawSanboxAreaLines")
        {
            fin >> m_drawSanboxAreaLines;
        }
        if (temp == "m_calibrationComplete")
        {
            fin >> m_calibrationComplete;
        }

        if (temp == "m_calibrationBoxComplete")
        {
            fin >> m_calibrationBoxComplete;
        }
    }

    generateWarpMatrix();
}

void Calibration::save(std::ofstream & fout)
{
    fout << "m_applyTransform" << " " << m_applyTransform << "\n";
    fout << "m_applyTransform2" << " " << m_applyTransform2 << "\n";
    fout << "m_drawSanboxAreaLines" << " " << m_drawSanboxAreaLines << "\n";

    fout << "m_calibrationComplete" << " " << m_calibrationComplete << "\n";
    fout << "m_calibrationBoxComplete" << " " << m_calibrationBoxComplete << "\n";

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