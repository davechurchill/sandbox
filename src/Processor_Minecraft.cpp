#include "Processor_Minecraft.h"

void Processor_Minecraft::init()
{
}

void Processor_Minecraft::imgui()
{
    m_minecraft.imgui();
}

void Processor_Minecraft::render(sf::RenderWindow & window)
{
}

void Processor_Minecraft::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
}

void Processor_Minecraft::save(std::ofstream & fout)
{
}

void Processor_Minecraft::load(const std::string & term, std::ifstream & fin)
{
}

void Processor_Minecraft::processTopography(const cv::Mat & data)
{
    m_minecraft.setGrid(data);
}
