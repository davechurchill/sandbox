#pragma once

#include "Save.hpp"
#include "TopographySource.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Source_Snapshot : public TopographySource
{
    cv::Mat m_snapshot;

    sf::Image m_image;
    sf::Texture m_texture;
    sf::Sprite m_sprite;

    void loadDataDump(const std::string & filename);
public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);

    cv::Mat getTopography();
};