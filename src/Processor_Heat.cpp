#include "Processor_Heat.h"
#include "Profiler.hpp"
#include "Tools.h"

#include "imgui.h"
#include "imgui-SFML.h"

namespace {
    const std::string shaderPath = "shaders/shader_heat.frag";
}

void Processor_Heat::init()
{
    m_shader.loadFromFile(shaderPath, sf::Shader::Fragment);
}

void Processor_Heat::imgui()
{
    PROFILE_FUNCTION();
    ImGui::Checkbox("Draw Projection", &m_drawProjection);

    if (ImGui::Button("Reload Shader"))
    {
        m_shader.loadFromFile(shaderPath, sf::Shader::Fragment);
    }

    /*
    const char* shaders[] = { "Popsicle", "Red", "Terrain", "Animating Water", "None" };
    ImGui::Combo("Color Scheme", &m_selectedShaderIndex, shaders, 5);
    */

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

        m_sfTransformedDepthSprite.setPosition(m_projector.getTransformedPosition());
        float scale = m_projector.getTransformedScale();
        m_sfTransformedDepthSprite.setScale(scale, scale);

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

void Processor_Heat::processEvent(const sf::Event& event, const sf::Vector2f& mouse)
{
    PROFILE_FUNCTION();
    m_projector.processEvent(event, mouse);
}

void Processor_Heat::save(Save& save) const
{
    save.selectedShaderIndex = m_selectedShaderIndex;
    save.drawContours = m_drawContours;
    save.numberOfContourLines = m_numberOfContourLines;
    save.drawProjection = m_drawProjection;
    m_projector.save(save);
}
void Processor_Heat::load(const Save& save)
{
    m_selectedShaderIndex = save.selectedShaderIndex;
    m_drawContours = save.drawContours;
    m_numberOfContourLines = save.numberOfContourLines;
    m_drawProjection = save.drawProjection;
    m_projector.load(save);
}

void Processor_Heat::processTopography(const cv::Mat& data)
{
    heatGrid.update(data);

    PROFILE_FUNCTION();
    {
        PROFILE_SCOPE("Calibration TransformProjection");
        m_projector.project(heatGrid.data(), m_cvTransformedDepthImage32f);
    }

    // Draw warped depth image
    int dw = m_cvTransformedDepthImage32f.cols;
    int dh = m_cvTransformedDepthImage32f.rows;

    // if something went wrong above, quit the function
    if (m_drawProjection && dw == 0 || dh == 0) { return; }

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
