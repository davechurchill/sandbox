#pragma once

#include "TopographyProcessor.h"
#include "MinecraftInterface.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Processor_Minecraft : public TopographyProcessor
{
    mc::MinecraftInterface m_minecraft;

public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);

    void processTopography(const cv::Mat & data, float deltaTime);
};