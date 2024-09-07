#include "Source_Snapshot.h"
#include "Tools.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include <fstream>
#include <filesystem>

void Source_Snapshot::init()
{
    loadDataDump("dataDumps/snapshot.bin");
}

void Source_Snapshot::imgui()
{
    ImGui::Text("Load:");

    ImGui::Indent();

    for (const auto & file : std::filesystem::directory_iterator("dataDumps/"))
    {
        std::string name = file.path().filename().string();
        if (ImGui::Button(name.c_str()))
        {
            loadDataDump(file.path().string());
        }
    }

    ImGui::Unindent();
}

void Source_Snapshot::render(sf::RenderWindow & window)
{
    window.draw(m_sprite);
}

void Source_Snapshot::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
}

void Source_Snapshot::loadDataDump(const std::string & filename)
{
    cv::FileStorage file(filename, cv::FileStorage::READ);
    file["matrix"] >> m_snapshot;

    m_image = Tools::matToSfImage(m_snapshot);
    m_texture.loadFromImage(m_image);
    m_sprite.setTexture(m_texture, true);
}

void Source_Snapshot::save(std::ofstream & fout)
{
}

void Source_Snapshot::load(const std::string & term, std::ifstream & fin)
{

}

cv::Mat Source_Snapshot::getTopography()
{
    return m_snapshot;
}
