#include "Processor_Heat.h"
#include "Profiler.hpp"
#include "Tools.h"

#include "imgui.h"
#include "imgui-SFML.h"

namespace {
    const std::string shaderPathColor = "shaders/shader_contour_color.frag";
    const std::string shaderPathHeat = "shaders/shader_heat.frag";
}

void Processor_Heat::init()
{
    m_shader_color.loadFromFile(shaderPathColor, sf::Shader::Fragment);
    m_shader_heat.loadFromFile(shaderPathHeat, sf::Shader::Fragment);
}

void Processor_Heat::imgui()
{
    PROFILE_FUNCTION();
    ImGui::Checkbox("Draw Projection", &m_drawProjection);

    if (ImGui::Button("Reload Shader"))
    {
        m_shader_color.loadFromFile(shaderPathColor, sf::Shader::Fragment);
        m_shader_heat.loadFromFile(shaderPathHeat, sf::Shader::Fragment);
    }

    ImGui::Checkbox("Draw Contour Lines", &m_drawContours);
    ImGui::InputInt("Contour Lines", &m_numberOfContourLines, 1, 10);

    static bool autoStep = false;

    if (ImGui::Button("Step") && !autoStep)
    {
        heatGrid.requestStep();
    }

    ImGui::Checkbox("Auto Step", &autoStep);

    if (autoStep)
    {
        heatGrid.requestStep();
    }

    if (ImGui::Button("Restart") && !autoStep)
    {
        heatGrid.restart();
        autoStep = false;
    }

    ImGui::Combo("Algorithm",
                 (int*)&heatGrid.algorithm,
                 HeatMap::AlgorithmNames,
                 IM_ARRAYSIZE(HeatMap::AlgorithmNames));

    ImGui::Spacing();

    m_projector.imgui();
}

void Processor_Heat::render(sf::RenderWindow& window)
{
    PROFILE_FUNCTION();
    if (m_drawProjection)
    {
        PROFILE_SCOPE("Draw Transformed Image");

        {
            m_sfTransformedDepthSpriteColor.setPosition(m_projector.getTransformedPosition());
            float scale = m_projector.getTransformedScale();
            m_sfTransformedDepthSpriteColor.setScale(scale, scale);

            static sf::Clock time;

            //Change color scheme
            m_shader_color.setUniform("shaderIndex", 1);
            m_shader_color.setUniform("contour", m_drawContours);
            m_shader_color.setUniform("numberOfContourLines", m_numberOfContourLines);
            m_shader_color.setUniform("u_time", time.getElapsedTime().asSeconds());

            //window.draw(m_sfTransformedDepthSpriteColor, &m_shader_color);
        }
        
        {
            m_sfTransformedDepthSpriteHeat.setPosition(m_projector.getTransformedPosition());
            float scale = m_projector.getTransformedScale();
            m_sfTransformedDepthSpriteHeat.setScale(scale, scale);

            //Change color scheme
            m_shader_heat.setUniform("contour", m_drawContours);
            m_shader_heat.setUniform("numberOfContourLines", m_numberOfContourLines);

            window.draw(m_sfTransformedDepthSpriteHeat, &m_shader_heat);
        }
    }

    m_projector.render(window);
}

void Processor_Heat::processEvent(const sf::Event& event, const sf::Vector2f& mouse)
{
    PROFILE_FUNCTION();
    m_projector.processEvent(event, mouse);
}

void Processor_Heat::save(Save& save) const
{
    save.drawContours = m_drawContours;
    save.numberOfContourLines = m_numberOfContourLines;
    save.drawProjection = m_drawProjection;
    m_projector.save(save);
}
void Processor_Heat::load(const Save& save)
{
    m_drawContours = save.drawContours;
    m_numberOfContourLines = save.numberOfContourLines;
    m_drawProjection = save.drawProjection;
    m_projector.load(save);
}

void Processor_Heat::processTopography(const cv::Mat& data)
{
    PROFILE_FUNCTION();

    {
        PROFILE_SCOPE("Color");

        {
            PROFILE_SCOPE("Calibration TransformProjection");
            m_projector.project(data, m_cvTransformedDepthImage32fColor);
        }

        // Draw warped depth image
        int dw = m_cvTransformedDepthImage32fColor.cols;
        int dh = m_cvTransformedDepthImage32fColor.rows;

        // if something went wrong above, quit the function
        if (m_drawProjection && dw == 0 || dh == 0) { return; }
        {
            {
                PROFILE_SCOPE("Transformed Image SFML Image");
                m_sfTransformedDepthImageColor = Tools::matToSfImage(m_cvTransformedDepthImage32fColor);

                {
                    PROFILE_SCOPE("SFML Texture From Image");
                    m_sfTransformedDepthTextureColor.loadFromImage(m_sfTransformedDepthImageColor);
                    m_sfTransformedDepthSpriteColor.setTexture(m_sfTransformedDepthTextureColor, true);
                }
            }
        }
    }

    {
        PROFILE_SCOPE("Heat");

        heatGrid.update(data);

        {
            PROFILE_SCOPE("Calibration TransformProjection");
            m_projector.project(heatGrid.data(), m_cvTransformedDepthImage32fHeat);
        }

        // Draw warped depth image
        int dw = m_cvTransformedDepthImage32fHeat.cols;
        int dh = m_cvTransformedDepthImage32fHeat.rows;

        // if something went wrong above, quit the function
        if (m_drawProjection && dw == 0 || dh == 0) { return; }
        {
            {
                PROFILE_SCOPE("Transformed Image SFML Image");
                m_sfTransformedDepthImageHeat = Tools::matToSfImage(m_cvTransformedDepthImage32fHeat);

                {
                    PROFILE_SCOPE("SFML Texture From Image");
                    m_sfTransformedDepthTextureHeat.loadFromImage(m_sfTransformedDepthImageHeat);
                    m_sfTransformedDepthSpriteHeat.setTexture(m_sfTransformedDepthTextureHeat, true);
                }
            }
        }
    }
}
