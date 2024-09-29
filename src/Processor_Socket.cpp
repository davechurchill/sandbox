#include "Processor_Socket.h"
#include "imgui.h"
#include "imgui-SFML.h"

#include <zmq_addon.hpp>

Processor_Socket::Processor_Socket() : m_socket(m_context, zmq::socket_type::push)
{
}

void Processor_Socket::init()
{
}

void Processor_Socket::imgui()
{
    if (!m_bound)
    {
        ImGui::InputInt("Port", &m_port);
        if (ImGui::Button("Connect"))
        {
            connect();
        }
    }
    else
    {
        if (ImGui::Button("Disconnect"))
        {
            m_socket.unbind(std::format("tcp://127.0.0.1:{}", m_port));
            m_bound = false;
        }
    }
}

void Processor_Socket::render(sf::RenderWindow & window)
{
}

void Processor_Socket::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
}

void Processor_Socket::save(Save & save) const
{
}

void Processor_Socket::load(const Save & save)
{
}

void Processor_Socket::connect()
{
    m_socket.bind(std::format("tcp://127.0.0.1:{}", m_port));
    m_bound = true;
}

void Processor_Socket::processTopography(const cv::Mat & data)
{
    if (m_bound && m_socket)
    {
        std::array<zmq::const_buffer, 3> messages = {
            zmq::buffer(&data.cols, sizeof(data.cols)), // width
            zmq::buffer(&data.rows, sizeof(data.rows)), // height
            zmq::buffer(data.ptr(), data.total() * data.elemSize()) // data
        };
        if (!zmq::send_multipart(m_socket, messages))
        {
            std::cout << "Failed to send messages" << std::endl;
        }
    }
}
