#include "SocketHandler.h"
#include "imgui.h"
#include "imgui-SFML.h"

#include <zmq_addon.hpp>
#include <thread>

SocketHandler::SocketHandler(cv::Mat* data) : m_socket(m_context, zmq::socket_type::router), m_data(data)
{
}

void SocketHandler::imgui()
{
    if (!m_running)
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
            m_running = false;
            m_thread.join();
        }
    }
}

void SocketHandler::connect()
{
    m_socket.bind(std::format("tcp://127.0.0.1:{}", m_port));
    m_running = true;
    m_thread = std::thread([this]()
        {
            zmq::message_t msg;
            m_socket.recv(msg);
            while (m_running) {
                if (msg.to_string() == "Data Pls") {
                    // Construct Message
                    std::array<zmq::const_buffer, 3> messages = {
                        zmq::buffer(&m_data->cols, sizeof(int)), // width
                        zmq::buffer(&m_data->rows, sizeof(int)), // height
                        zmq::buffer(&m_data->data, m_data->total() * m_data->elemSize()) // data
                    };

                    // Send Message
                    if (!zmq::send_multipart(m_socket, messages))
                    {
                        std::cout << "Failed to send messages" << std::endl;
                    }
                }
                m_socket.recv(msg);
            }
        });
}