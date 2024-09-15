#pragma once

#include "TopographyProcessor.h"

#include <asio.hpp>

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Processor_Socket : public TopographyProcessor
{
    asio::io_context m_context;
    asio::ip::tcp::socket m_socket;
    int m_port = 4960;

public:
    Processor_Socket();
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);
    void connect();

    void processTopography(const cv::Mat & data);
};