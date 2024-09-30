#pragma once

#include "TopographyProcessor.h"

#include <zmq.hpp>
#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Processor_Socket : public TopographyProcessor
{
    int m_port = 4960;
    bool m_bound = false;
    zmq::context_t m_context;
    zmq::socket_t m_socket;

    bool m_limitFrames = false;
    int m_frameRate = 60;
    char m_toggleFrames = 0; // 0 is do nothing, 1 is activate framelimit, 2 is disable it

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