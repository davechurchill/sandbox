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
        m_pointCircles.push_back(c);
    }

    float radius2 = 5.0;
    for (int i = 0; i < 4; ++i)
    {
        sf::CircleShape c(radius2);
        c.setOrigin(radius2, radius2);
        c.setFillColor(sf::Color::Magenta);
        c.setPosition(-3247, -3247);
        m_pointBoxCircles.push_back(c);
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
               m_pointCircles[i].setPosition(-3247, -3247);
            }
        }

        if (ImGui::Button("Select Box Corners"))
        {
            m_currentBoxPoint = 0;
            m_calibrationBoxComplete = false;

            for (int i = 0; i < 4; ++i)
            {
                m_pointBoxCircles[i].setPosition(-3247, -3247);
            }
        }

        if (m_calibrationComplete && ImGui::Button("Auto Sort Corners"))
        {
            orderPoints();
            generateWarpMatrix();
        }
    }
    bool w = ImGui::InputInt("Width", &m_width);
    bool h = ImGui::InputInt("Height", &m_height);
    if (w || h)
    {
        generateWarpMatrix();
    }
    ImGui::Text("Width: %d", m_width);
    ImGui::Text("Height: %d", m_height);

    if (m_calibrationComplete)
    {
        ImGui::Checkbox("Apply Transform", &m_applyTransform);
        ImGui::Checkbox("Apply Transform2", &m_applyTransform2);
        ImGui::Checkbox("Apply Height Adjustment", &m_applyAdjustment);
    }

    if (m_applyTransform2)
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
        float ratio = (float) m_boxHeight / m_boxWidth;
        int width = m_width * 1.5f;
        int height = width * ratio;
        cv::warpPerspective(input, output, m_boxOperator, cv::Size(width, height));
    }
}

void Calibration::heightAdjustment(cv::Mat & matrix)
{
    if (m_applyTransform && m_calibrationComplete && m_applyAdjustment)
    {
        int width = matrix.cols;
        int height = matrix.rows;

        float topLeft = matrix.at<float>(0, 0);

        int centerX = width / 2;
        int centerY = height / 2;

        float centerValue = matrix.at<float>(centerY, centerX);

        float bottomRight = matrix.at<float>(0, width - 1);

        float vect_A[] = { centerX, centerY, centerValue - topLeft };
        float vect_B[] = { width - 1, 0, bottomRight - topLeft };
        float cross_P[] = { 0.0, 0.0, 0.0 };

        cross_P[0] = vect_A[1] * vect_B[2] - vect_A[2] * vect_B[1];
        cross_P[1] = vect_A[2] * vect_B[0] - vect_A[0] * vect_B[2];
        cross_P[2] = vect_A[0] * vect_B[1] - vect_A[1] * vect_B[0];

        //plane equation
        float d = -(cross_P[0] * centerX + cross_P[1] * centerY + cross_P[2] * centerValue);

        for (size_t i = 0; i < width; i++)
        {
            for (size_t j = 0; j < height; j++)
            {
                float newZ = (-d - cross_P[0] * i - cross_P[1] * j) / cross_P[2];
                matrix.at<float>(j, i) = newZ;
            }
        }
    }
}

void Calibration::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    if (m_dragPoint != -1) {
        m_points[m_dragPoint] = cv::Point(mouse.x, mouse.y);
        m_pointCircles[m_dragPoint].setPosition(mouse);
    }

    if (m_dragBoxPoint != -1) {
        m_boxPoints[m_dragBoxPoint] = cv::Point(mouse.x, mouse.y);
        m_pointBoxCircles[m_dragBoxPoint].setPosition(mouse);
    }

    if (m_calibrationComplete == true && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        int i = 0;
        while (i < 4)
        {
            float xDistance = mouse.x - m_pointCircles[i].getPosition().x;
            float yDistance = mouse.y - m_pointCircles[i].getPosition().y;
            float pointsDistance = sqrt((xDistance * xDistance) + (yDistance * yDistance));

            if (pointsDistance <= m_pointCircles[i].getRadius())
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
            float xDistance = mouse.x - m_pointBoxCircles[i].getPosition().x;
            float yDistance = mouse.y - m_pointBoxCircles[i].getPosition().y;
            float pointsDistance = sqrt((xDistance * xDistance) + (yDistance * yDistance));

            if (pointsDistance <= m_pointBoxCircles[i].getRadius())
            {
                m_dragBoxPoint = i;
            }
            i++;
        }
    }

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

    if (m_currentBoxPoint > -1 && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        std::cout << mouse.x << " " << mouse.y << std::endl;
        m_boxPoints[m_currentBoxPoint] = cv::Point(mouse.x, mouse.y);
        m_pointBoxCircles[m_currentBoxPoint].setPosition(mouse);
        if (++m_currentBoxPoint > 3)
        {
            m_currentBoxPoint = -1;
            m_calibrationBoxComplete = true;
            generateWarpMatrix();
        }
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
    {
        m_dragPoint = -1;
        generateWarpMatrix();
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
    {
        m_dragBoxPoint = -1;
        generateWarpMatrix();
    }

}

void Calibration::render(sf::RenderWindow & window)
{
    //if (!m_applyTransform)
    {
        int n = m_calibrationComplete ? m_pointCircles.size() : m_currentPoint;
        for (int i = 0; i < n; ++i)
        {
            window.draw(m_pointCircles[i]);
        }

        int nBox = m_calibrationBoxComplete ? m_pointBoxCircles.size() : m_currentBoxPoint;
        for (int i = 0; i < nBox; ++i)
        {
            if (m_drawSanboxAreaLines)
            {
                window.draw(m_pointBoxCircles[i]);
            }
        }
    }

    if (m_pointCircles[1].getPosition().x != -3247)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_pointCircles[0].getPosition().x, m_pointCircles[0].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_pointCircles[1].getPosition().x, m_pointCircles[1].getPosition().y))
        };
        window.draw(line, 2, sf::Lines);
    }

    if (m_pointBoxCircles[1].getPosition().x != -3247  && m_drawSanboxAreaLines)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_pointBoxCircles[0].getPosition().x, m_pointBoxCircles[0].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_pointBoxCircles[1].getPosition().x, m_pointBoxCircles[1].getPosition().y))
        };
        window.draw(line, 2, sf::Lines);
    }

    if (m_pointCircles[2].getPosition().x != -3247)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_pointCircles[0].getPosition().x, m_pointCircles[0].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_pointCircles[2].getPosition().x, m_pointCircles[2].getPosition().y))
        };
        window.draw(line, 2, sf::Lines);
    }

    if (m_pointBoxCircles[2].getPosition().x != -3247 && m_drawSanboxAreaLines)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_pointBoxCircles[0].getPosition().x, m_pointBoxCircles[0].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_pointBoxCircles[2].getPosition().x, m_pointBoxCircles[2].getPosition().y))
        };
        window.draw(line, 2, sf::Lines);
    }

    if (m_calibrationComplete )
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_pointCircles[1].getPosition().x, m_pointCircles[1].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_pointCircles[3].getPosition().x, m_pointCircles[3].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_pointCircles[2].getPosition().x, m_pointCircles[2].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_pointCircles[3].getPosition().x, m_pointCircles[3].getPosition().y))
        };
        window.draw(line, 4, sf::Lines);
    }

    if (m_calibrationBoxComplete && m_drawSanboxAreaLines)
    {
        sf::Vertex line[] =
        {
            sf::Vertex(sf::Vector2f(m_pointBoxCircles[1].getPosition().x, m_pointBoxCircles[1].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_pointBoxCircles[3].getPosition().x, m_pointBoxCircles[3].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_pointBoxCircles[2].getPosition().x, m_pointBoxCircles[2].getPosition().y)),
            sf::Vertex(sf::Vector2f(m_pointBoxCircles[3].getPosition().x, m_pointBoxCircles[3].getPosition().y))
        };
        window.draw(line, 4, sf::Lines);
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

std::vector<cv::Point2f>  Calibration::getConfig()
{
    std::vector<cv::Point2f> points;
    points.push_back(m_points[0]);
    points.push_back(m_points[1]);
    points.push_back(m_points[2]);
    points.push_back(m_points[3]);
    return points;
}

std::vector<sf::CircleShape> Calibration::getPointCircle()
{
    return m_pointCircles;
}

std::vector<cv::Point2f>  Calibration::getBoxConfig()
{
    std::vector<cv::Point2f> boxPoints;
    boxPoints.push_back(m_boxPoints[0]);
    boxPoints.push_back(m_boxPoints[1]);
    boxPoints.push_back(m_boxPoints[2]);
    boxPoints.push_back(m_boxPoints[3]);
    return boxPoints;
}

std::vector<sf::CircleShape> Calibration::getPointBoxCircle()
{
    return m_pointBoxCircles;
}


cv::Point2f Calibration::getDimension()
{
    return cv::Point2f(m_width, m_height);
}

cv::Point2f Calibration::getBoxDimension()
{
    return cv::Point2f(m_boxWidth, m_boxHeight);
}

void Calibration::loadConfiguration()
{
    std::ifstream fin("config.txt");
    std::string temp;
    while (fin >> temp)
    {
        if (temp == "m_points[0]")
        {
            fin >> m_points[0].x;
            fin >> m_points[0].y;
        }

        if (temp == "m_boxPoints[0]")
        {
            fin >> m_boxPoints[0].x;
            fin >> m_boxPoints[0].y;
        }

        if (temp == "m_points[1]")
        {
            fin >> m_points[1].x;
            fin >> m_points[1].y;
        }

        if (temp == "m_boxPoints[1]")
        {
            fin >> m_boxPoints[1].x;
            fin >> m_boxPoints[1].y;
        }

        if (temp == "m_points[2]")
        {
            fin >> m_points[2].x;
            fin >> m_points[2].y;
        }

        if (temp == "m_boxPoints[2]")
        {
            fin >> m_boxPoints[2].x;
            fin >> m_boxPoints[2].y;
        }

        if (temp == "m_points[3]")
        {
            fin >> m_points[3].x;
            fin >> m_points[3].y;
        }

        if (temp == "m_boxPoints[3]")
        {
            fin >> m_boxPoints[3].x;
            fin >> m_boxPoints[3].y;
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
            m_pointCircles[0].setPosition(x, y);
        }

        if (temp == "m_pointCircles[1]")
        {
            float x, y;
            fin >> x >> y;
            m_pointCircles[1].setPosition(x, y);
        }

        if (temp == "m_pointCircles[2]")
        {
            float x, y;
            fin >> x >> y;
            m_pointCircles[2].setPosition(x, y);
        }

        if (temp == "m_pointCircles[3]")
        {
            float x, y;
            fin >> x >> y;
            m_pointCircles[3].setPosition(x, y);
        }

        if (temp == "m_pointBoxCircles[0]")
        {
            float x, y;
            fin >> x >> y;
            m_pointBoxCircles[0].setPosition(x, y);
        }

        if (temp == "m_pointBoxCircles[1]")
        {
            float x, y;
            fin >> x >> y;
            m_pointBoxCircles[1].setPosition(x, y);
        }

        if (temp == "m_pointBoxCircles[2]")
        {
            float x, y;
            fin >> x >> y;
            m_pointBoxCircles[2].setPosition(x, y);
        }

        if (temp == "m_pointBoxCircles[3]")
        {
            float x, y;
            fin >> x >> y;
            m_pointBoxCircles[3].setPosition(x, y);
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

    //m_calibrationComplete = true;
    //m_calibrationBoxComplete = true;
    //m_applyTransform = true;
    //m_applyTransform2 = true;
    generateWarpMatrix();
}

void Calibration::generateWarpMatrix()
{
    cv::Point2f dstPoints[] = {
            cv::Point2f(0, 0),
            cv::Point2f(m_width, 0),
            cv::Point2f(0, m_height),
            cv::Point2f(m_width, m_height),
    };
    std::cout <<"m_point print kortesi" << "\n";
    std::cout << m_points[0].x << " " << m_points[0].y << "\n";
    std::cout << m_points[1].x << " " << m_points[1].y << "\n";
    std::cout << m_points[2].x << " " << m_points[2].y << "\n";
    std::cout << m_points[3].x << " " << m_points[3].y << "\n";

    std::cout << "m_boxPoint print kortesi" << "\n";
    std::cout << m_boxPoints[0].x << " " << m_boxPoints[0].y << "\n";
    std::cout << m_boxPoints[1].x << " " << m_boxPoints[1].y << "\n";
    std::cout << m_boxPoints[2].x << " " << m_boxPoints[2].y << "\n";
    std::cout << m_boxPoints[3].x << " " << m_boxPoints[3].y << "\n";

    m_operator = cv::getPerspectiveTransform(m_points, dstPoints);

    cv::Point2f boxPoints[] = { m_boxPoints [0], m_boxPoints[1], m_boxPoints[2], m_boxPoints[3]};

    tempX = boxPoints[0].x;
    tempY = boxPoints[0].y;
    float maxX = boxPoints[0].x;
    float maxY = boxPoints[0].y;
    for (int i = 0; i < 4; i++)
    {
        if (boxPoints[i].x < tempX)
        {
            tempX = boxPoints[i].x;
        }
        if (boxPoints[i].x > maxX)
        {
            maxX = boxPoints[i].x;
        }

        if (boxPoints[i].y < tempY)
        {
            tempY = boxPoints[i].y;
        }

        if (boxPoints[i].y > maxY)
        {
            maxY = boxPoints[i].y;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        boxPoints[i].x -= tempX;
        boxPoints[i].y -= tempY;
    }
    m_boxWidth = maxX - tempX;
    m_boxHeight = maxY - tempY;
    m_boxOperator = cv::getPerspectiveTransform(dstPoints, boxPoints);
}