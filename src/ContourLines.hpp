#pragma once

#include "Grid.hpp"
#include "Scene.h"
#include "iostream"
#include <SFML/Graphics.hpp>


class ContourLines
{
    Grid<char> m_onContour;
    float m_contourLevel = 0.5;
    int   m_numberOfContourLines = 5;
    std::vector <float> m_contourLevels;
    sf::Image m_linesImage;
    sf::Texture m_linesTexture;

public:

    ContourLines()
    {
        float step = 1.0f / (m_numberOfContourLines + 1);
        for (float contourLine = 0; contourLine <= 1.0f; contourLine += step)
        {
            m_contourLevels.push_back(contourLine);
        }
    }

    void init(size_t width, size_t height)
    {
        m_onContour = Grid<char>(width, height, 0);
    }

    void setContourLevel(float c)
    {
        m_contourLevel = c;
    }

    void setNumberofContourLines(int n)
    {
        m_onContour.clear(0);
        m_contourLevels.clear();
        m_numberOfContourLines = n;
        float step = 1.0f / (m_numberOfContourLines + 1);
        for (float contourLine = 0; contourLine <= 1.0f; contourLine += step)
        {
            m_contourLevels.push_back(contourLine);
        }
    }

    bool isOnContour(size_t x, size_t y)
    {
        return m_onContour.get(x, y) == 1;
    }

    //heightGrid has all the floating values that will help to compare with the contourlevel.
    void calculate(const Grid<float> & heightGrid)
    {
        for (auto& contourLevel : m_contourLevels)
        {
            m_contourLevel = contourLevel;

            // daw grid cells with the associated colors
            for (size_t x = 0; x < heightGrid.width(); x++)
            {
                for (size_t y = 0; y < heightGrid.height(); y++)
                {
                    float cellHeight = heightGrid.get(x, y);
                    //std::cout << cellHeight << '\n';

                    // if the current cell value is greater than the contour level
                    //    if one of its neighbours is less than the contour level
                    //       then the current cell is considered on the contour

                    if (cellHeight > m_contourLevel)
                    {
                        if (y > 0 and heightGrid.get(x, y - 1) <= m_contourLevel)
                        {
                            m_onContour.set(x, y, 1);
                        }

                        if (y < heightGrid.height() - 1 and heightGrid.get(x, y + 1) <= m_contourLevel)
                        {
                            m_onContour.set(x, y, 1);
                        }

                        if (x > 0 and heightGrid.get(x - 1, y) <= m_contourLevel)
                        {
                            m_onContour.set(x, y, 1);
                        }

                        if (x < heightGrid.width() - 1 and heightGrid.get(x + 1, y) <= m_contourLevel)
                        {
                            m_onContour.set(x, y, 1);
                        }
                    }
                }
            }
        }
    }

    sf::Texture & generateTexture()
    {
        size_t width = m_onContour.width();
        size_t height = m_onContour.height();
        m_linesImage.create(width, height, sf::Color::Transparent);

        for (int i = 0; i < width; ++i)
        {
            for (int j = 0; j < height; ++j)
            {
                if (isOnContour(i, j))
                {
                    m_linesImage.setPixel(i, j, sf::Color::White);
                }
            }
        }

        m_linesTexture.loadFromImage(m_linesImage);

        return m_linesTexture;
    }
};