#include "Processor_Colorizer.h"
#include "Profiler.hpp"
#include "Tools.h"

#include "imgui.h"
#include "imgui-SFML.h"

void Processor_Colorizer::init()
{
    m_shader.loadFromFile("shaders/shader_contour_color.frag", sf::Shader::Fragment);
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
        m_shader.loadFromFile("shaders/shader_contour_color.frag", sf::Shader::Fragment);
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
        m_sfTransformedDepthSprite.setScale(scale, scale);

        //Use static so that it does not get initilialized every time this function is called
        static sf::Clock time;

        //Change color scheme
        m_shader.setUniform("shaderIndex", m_selectedShaderIndex);
        m_shader.setUniform("contour", m_drawContours);
        m_shader.setUniform("numberOfContourLines", m_numberOfContourLines);
        m_shader.setUniform("u_time", time.getElapsedTime().asSeconds());

        window.draw(m_sfTransformedDepthSprite, &m_shader);
    }

    m_projector.render(window);
}

void Processor_Colorizer::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    PROFILE_FUNCTION();
    m_projector.processEvent(event, mouse);
}

void Processor_Colorizer::save(Save & save) const
{
    save.selectedShaderIndex = m_selectedShaderIndex;
    save.drawContours = m_drawContours;
    save.numberOfContourLines = m_numberOfContourLines;
    m_projector.save(save);
}
void Processor_Colorizer::load(const Save & save)
{
    m_selectedShaderIndex = save.selectedShaderIndex;
    m_drawContours = save.drawContours;
    m_numberOfContourLines = save.numberOfContourLines;
    m_projector.load(save);
}

void Processor_Colorizer::processTopography(const cv::Mat & data)
{
    PROFILE_FUNCTION();
    {
        PROFILE_SCOPE("Calibration TransformProjection");
        m_projector.project(data, m_cvTransformedDepthImage32f);
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
                m_sfTransformedDepthTexture.loadFromImage(m_sfTransformedDepthImage);
                m_sfTransformedDepthSprite.setTexture(m_sfTransformedDepthTexture, true);
            }
        }
    }
}
