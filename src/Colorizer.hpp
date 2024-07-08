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
        Popsicle,
        Terrain
    };

    Modes mode = RedWhite;
    float water = 0.4f;
    float snow = 0.8f;

    void color(sf::Image & output, const Grid<float> & input)
    {
        sf::Color (Colorizer::* func)(float);
        switch (mode)
        {
            case Greyscale: func = &Colorizer::greyscale; break;
            case RedWhite: func = &Colorizer::redwhite; break;
            case Popsicle: func = &Colorizer::popsicle; break;
            case Terrain: func = &Colorizer::terrain; break;
        }

        output.create((unsigned int)input.width(), (unsigned int)input.height());
        for (unsigned int i = 0; i < input.width(); ++i)
        {
            for (unsigned int j = 0; j < input.height(); ++j)
            {
                float height = input.get(i,j);
                if (height < 0.000001f || height > 0.999999f)
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
        const char * options[] = { "Greyscale", "RedWhite", "Popsicle", "Terrain"};
        ImGui::Combo("ColorMode", (int *)&mode, options, 4);
        if (mode == 3)
        {
            ImGui::Indent();
            ImGui::SliderFloat("Water Level", &water, 0.0f, 1.0f);
            ImGui::SliderFloat("Snow Level", &snow, 0.0f, 1.0f);
            ImGui::Unindent();
        }
    }

private:
    sf::Color popsicle(float height)
    {
        int dNormal = (int)((1.0f - height) * 1529.f);
        int pR, pG, pB;
        int i = dNormal / 255;
        switch (i)
        {
        case 0: { pR = 255; pG = dNormal; pB = dNormal; } break;
        case 1: { pR = 510 - dNormal; pG = 255; pB = 510 - dNormal; } break;
        case 2: { pR = dNormal - 510; pG = 765 - dNormal; pB = dNormal - 510; } break;
        case 3: { pR = 1020 - dNormal; pG = dNormal - 765; pB = 255; } break;
        case 4: { pR = 0; pG = 1275 - dNormal; pB = 255; } break;
        case 5: { pR = 0; pG = 0; pB = 1529 - dNormal; } break;
        }
        return sf::Color(pR, pG, pB);
    }

    sf::Color greyscale(float height)
    {
        sf::Uint8 val = (sf::Uint8)(255.0f * height);
        return sf::Color(val, val, val);
    }

    sf::Color redwhite(float height)
    {
        sf::Uint8 val = (sf::Uint8)(255.0f * height);
        return sf::Color(255, val, val);
    }

    sf::Color terrain(float height)
    {
        if (height >= snow)
        {
            int s = (int)(255 * height);
            return sf::Color(s, s, s);  // White snow
        }
        if (height >= water)
        {
            return sf::Color(0, (sf::Uint8)(255.f * height), 0); // Green ground
        }
        float w = 1.0f - (water - height) / water;
        return sf::Color(0, 0, sf::Uint8(255.f * w)); // Blue Water
    }
};