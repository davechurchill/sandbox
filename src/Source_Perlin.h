#pragma once

#include "Save.hpp"
#include "TopographySource.h"
#include "Perlin.hpp"
#include "Grid.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Source_Perlin : public TopographySource
{
    Perlin2DNew         m_perlin;
    int                 m_octaves = 5;
    int                 m_seed = 0;
    int                 m_seedSize = 9;
    float               m_persistance = 0.5f;
    bool                m_drawGrid = false;
    Grid<float>         m_grid;

    cv::Mat             m_topography;

    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite;


    void calculateNoise();

public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);

    cv::Mat getTopography();
};