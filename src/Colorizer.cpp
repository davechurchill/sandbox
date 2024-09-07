#include "Colorizer.h"
#include "Profiler.hpp"
#include "Tools.h"

#include "imgui.h"
#include "imgui-SFML.h"

void Colorizer::init()
{
    m_shader.loadFromFile("shaders/shader_contour_color.frag", sf::Shader::Fragment);
}

void Colorizer::imgui()
{
    PROFILE_FUNCTION();
    ImGui::Checkbox("Draw Projection", &m_drawProjection);

    if (ImGui::Button("Reload Shader"))
    {
        m_shader.loadFromFile("shaders/shader_contour_color.frag", sf::Shader::Fragment);
    }

    const char * shaders[] = { "Popsicle", "Red", "Terrain", "Animating Water", "None" };
    ImGui::Combo("Color Scheme", &m_selectedShaderIndex, shaders, 5);

    ImGui::Checkbox("Draw Contour Lines", &m_drawContours);
    ImGui::InputInt("Contour Lines", &m_numberOfContourLines, 1, 10);

    ImGui::Spacing();

    m_projector.imgui();
}

void Colorizer::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();
    if (m_drawProjection)
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

void Colorizer::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    PROFILE_FUNCTION();
    m_projector.processEvent(event, mouse);
}

void Colorizer::save(std::ofstream & fout)
{
    fout << "m_drawProjection " << m_drawProjection << '\n';
    fout << "m_selectedShaderIndex " << m_selectedShaderIndex << '\n';
    fout << "m_drawContours " << m_drawContours << '\n';
    fout << "m_numberOfContourLines " << m_numberOfContourLines << '\n';

    m_projector.save(fout);
}

void Colorizer::load(const std::string & term, std::ifstream & fin)
{
    if (term == "m_drawProjection") { fin >> m_drawProjection; }
    if (term == "m_selectedShaderIndex") { fin >> m_selectedShaderIndex; }
    if (term == "m_drawContours") { fin >> m_drawContours; }
    if (term == "m_numberOfContourLines") { fin >> m_numberOfContourLines; }

    m_projector.load(term, fin);
}

void Colorizer::processTopography(const cv::Mat & data)
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
