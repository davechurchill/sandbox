#pragma once

#include "Grid.hpp"
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"

struct Colorizer
{
    enum Modes
    {
        Greyscale,
        RedWhite,
        Popsicle
    };

    Modes mode = Greyscale;

    void color(sf::Image & output, const Grid<float> & input)
    {
        sf::Color (Colorizer::* func)(float);
        switch (mode)
        {
        case Greyscale: func = &Colorizer::greyscale; break;
        case RedWhite: func = &Colorizer::redwhite; break;
        case Popsicle: func = &Colorizer::popsicle; break;
        }

        output.create(input.width(), input.height());
        for (int i = 0; i < input.width(); ++i)
        {
            for (int j = 0; j < input.height(); ++j)
            {
                float height = input.get(i,j);
                if (height < 0.0f)
                {
                    output.setPixel(i, j, sf::Color::Black);
                    continue;
                }
                
                output.setPixel(i, j, (this->*(func))(height));
            }
        }
    }

    void imgui()
    {
        const char * options[] = { "Greyscale", "RedWhite", "Popsicle" };
        ImGui::Combo("ColorMode", (int *)&mode, options, 3);
    }

private:
    sf::Color popsicle(float height)
    {
        int dNormal = height * 1529.f;
        int pR, pG, pB;
        int i = dNormal / 255;
        switch (i)
        {
        case 0: { pR = 255; pG = dNormal; pB = dNormal; } break;
        case 1: { pR = dNormal - 255; pG = 255; pB = dNormal; } break;
        case 2: { pR = 0; pG = 765 - dNormal; pB = dNormal - 510; } break;
        case 3: { pR = 0; pG = 0; pB = dNormal - 765; } break;
        case 4: { pR = dNormal - 1020; pG = 0; pB = 255; } break;
        case 5: { pR = 255; pG = 0; pB = 1529 - dNormal; } break;
        }
        return sf::Color(pR, pG, pB);
    }

    sf::Color greyscale(float height)
    {
        return sf::Color(255.f * height, 255.f * height, 255.f * height);
    }

    sf::Color redwhite(float height)
    {
        return sf::Color(255, 255.f * height, 255.f * height);
    }
};