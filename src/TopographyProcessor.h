#pragma once

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class TopographyProcessor
{
public:
    virtual void init() = 0;
    virtual void imgui() = 0;
    virtual void render(sf::RenderWindow & window) = 0;
    virtual void processEvent(const sf::Event & event, const sf::Vector2f & mouse) = 0;
    virtual void save(std::ofstream & fout) = 0;
    virtual void load(const std::string & term, std::ifstream & fin) = 0;

    virtual void processTopography(const cv::Mat & data) = 0;

    enum
    {
        Colorizer,
    };


};