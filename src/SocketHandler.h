#pragma once

#include <zmq.hpp>
#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class SocketHandler
{
    int m_port = 4960;
    bool m_running = false;
    zmq::context_t m_context;
    zmq::socket_t m_socket;
    std::thread m_thread;
    cv::Mat* m_data;


public:
    SocketHandler(cv::Mat * data);
    void imgui();
    void connect();
    void stop();
    ~SocketHandler();
};