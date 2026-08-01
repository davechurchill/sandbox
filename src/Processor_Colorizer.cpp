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
    m_projector.imgui();
}

void Processor_Colorizer::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    {
        PROFILE_SCOPE("Draw Transformed Image");

        m_sfTransformedDepthSprite.setPosition(m_projector.getTransformedPosition());
        float scale = m_projector.getTransformedScale();
        m_sfTransformedDepthSprite.setScale({ scale, scale });

        //Use static so that it does not get initialized every time this function is called
        static sf::Clock time;

        //Change color scheme
        m_shader.setUniform("shaderIndex", m_selectedShaderIndex);
        m_shader.setUniform("contour", m_drawContours);
        m_shader.setUniform("numberOfContourLines", m_numberOfContourLines);
        m_shader.setUniform("u_time", time.getElapsedTime().asSeconds());

        window.draw(m_sfTransformedDepthSprite, &m_shader);
    }

}

void Processor_Colorizer::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    PROFILE_FUNCTION();
    m_projector.processEvent(event, mouse);
}

void Processor_Colorizer::save(Save & save) const
{
    Save::Json & settings = save.section("Processor_Colorizer");
    settings["m_selectedShaderIndex"] = m_selectedShaderIndex;
    settings["m_drawContours"] = m_drawContours;
    settings["m_numberOfContourLines"] = m_numberOfContourLines;
    m_projector.save(save);
}
void Processor_Colorizer::load(const Save & save)
{
    const Save::Json & settings = save.section("Processor_Colorizer");
    Save::read(settings, "m_selectedShaderIndex", m_selectedShaderIndex);
    Save::read(settings, "m_drawContours", m_drawContours);
    Save::read(settings, "m_numberOfContourLines", m_numberOfContourLines);
    m_projector.load(save);
}

void Processor_Colorizer::processTopography(const IntermediateData& data)
{
    PROFILE_FUNCTION();
    {
        PROFILE_SCOPE("Calibration TransformProjection");
        m_projector.project(data.topography, m_cvTransformedDepthImage32f);
    }

    // Draw warped depth image
    int dw = m_cvTransformedDepthImage32f.cols;
    int dh = m_cvTransformedDepthImage32f.rows;

    // if something went wrong above, quit the function
    if (dw == 0 || dh == 0) { return; }
    {
        {
            PROFILE_SCOPE("Transformed Image SFML Image");
            m_sfTransformedDepthImage = Tools::matToSfImage(m_cvTransformedDepthImage32f);

            {
                PROFILE_SCOPE("SFML Texture From Image");
                if (!m_sfTransformedDepthTexture.loadFromImage(m_sfTransformedDepthImage))
                {
                    std::cerr << "Failed to load the colorizer terrain texture.\n";
                }
                else
                {
                    m_sfTransformedDepthSprite.setTexture(m_sfTransformedDepthTexture, true);
                }
            }
        }
    }
}
