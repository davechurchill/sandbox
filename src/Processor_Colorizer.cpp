#include "Processor_Colorizer.h"
#include "Profiler.hpp"
#include "Tools.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include <iostream>

void Processor_Colorizer::init()
{
    if (!m_shader.loadFromFile("shaders/shader_contour_color.frag", sf::Shader::Type::Fragment))
    {
        std::cerr << "Failed to load the colorizer shader.\n";
    }
}

void Processor_Colorizer::imgui()
{
    PROFILE_FUNCTION();

    const char * shaders[] = { "Popsicle", "Blue", "Red", "Terrain", "Animating Water", "None" };
    ImGui::Combo("Color Scheme", &m_selectedShaderIndex, shaders, 5);

    ImGui::Checkbox("##Contours", &m_drawContours);
    ImGui::SameLine();
    ImGui::SliderInt("Contour Lines", &m_numberOfContourLines, 0, 19);

    ImGui::Separator();

    if (ImGui::Button("Reload Shader"))
    {
        if (!m_shader.loadFromFile("shaders/shader_contour_color.frag", sf::Shader::Type::Fragment))
        {
            std::cerr << "Failed to reload the colorizer shader.\n";
        }
    }
}

void Processor_Colorizer::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    {
        PROFILE_SCOPE("Draw Transformed Image");

        m_sprite.setPosition(projector().getTransformedPosition());
        const float scale = projector().getTransformedScale();
        m_sprite.setScale({ scale, scale });

        //Use static so that it does not get initialized every time this function is called
        static sf::Clock time;

        //Change color scheme
        m_shader.setUniform("shaderIndex", m_selectedShaderIndex);
        m_shader.setUniform("contour", m_drawContours);
        m_shader.setUniform("numberOfContourLines", m_numberOfContourLines);
        m_shader.setUniform("u_time", time.getElapsedTime().asSeconds());

        window.draw(m_sprite, &m_shader);
    }

}

void Processor_Colorizer::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    PROFILE_FUNCTION();
    projector().processEvent(event, mouse);
}

void Processor_Colorizer::save(Settings & save) const
{
    Settings::json & settings = save.section("Processor_Colorizer");
    settings["m_selectedShaderIndex"] = m_selectedShaderIndex;
    settings["m_drawContours"] = m_drawContours;
    settings["m_numberOfContourLines"] = m_numberOfContourLines;
}
void Processor_Colorizer::load(const Settings & save)
{
    const Settings::json & settings = save.section("Processor_Colorizer");
    Settings::read(settings, "m_selectedShaderIndex", m_selectedShaderIndex);
    Settings::read(settings, "m_drawContours", m_drawContours);
    Settings::read(settings, "m_numberOfContourLines", m_numberOfContourLines);
}

void Processor_Colorizer::processTopography(const IntermediateData& data)
{
    PROFILE_FUNCTION();
    if (!Tools::updateProjectedTexture(
            data.topography,
            projector(),
            m_projectedTopography,
            m_image,
            m_texture,
            m_sprite,
            false,
            "Failed to load the colorizer terrain texture.\n"))
    {
        return;
    }
}
