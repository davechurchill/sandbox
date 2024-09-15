#include "Processor_Socket.h"
#include "imgui.h"
#include "imgui-SFML.h"

using namespace asio::ip;

Processor_Socket::Processor_Socket() : m_socket(m_context)
{
}

void Processor_Socket::init()
{
}

void Processor_Socket::imgui()
{
    if (!m_socket.is_open())
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
            m_socket.close();
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
    asio::error_code er;
    m_socket.connect(tcp::endpoint(address::from_string("127.0.0.1"), m_port), er);
    if (er)
    {
        std::cout << er.message() << std::endl;
        m_socket.close();
    }
    std::cout << m_socket.is_open() << std::endl;
}

void Processor_Socket::processTopography(const cv::Mat & data)
{
    if (m_socket.is_open())
    {
        asio::error_code er;
        m_socket.send(asio::buffer("hi"), asio::socket_base::message_flags(), er);
        if (er)
        {
            std::cout << er.message() << std::endl;
        }
    }
}
