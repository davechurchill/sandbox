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
    setInitialHeatSources();
    m_shader_color.loadFromFile(shaderPathColor, sf::Shader::Fragment);
    m_shader_heat.loadFromFile(shaderPathHeat, sf::Shader::Fragment);
}

void Processor_Heat::setInitialHeatSources()
{
    m_heatGrid.clearSources();
    m_heatGrid.addSource(HeatSource(cv::Rect(100, 100, 10, 10), 100.0f));
    m_heatGrid.addSource(HeatSource(cv::Rect(300, 100, 10, 10), -100.0f));
    m_heatGrid.addSource(HeatSource(cv::Rect(300, 200, 10, 10), 100.0f));
    m_heatGrid.addSource(HeatSource(cv::Rect(100, 200, 10, 10), 100.0f));
}

void Processor_Heat::imgui()
{
    PROFILE_FUNCTION();

    // Common options
    {
        m_projector.imgui();
        ImGui::Checkbox("Draw Projection", &m_drawProjection);

        if (ImGui::Button("Reload Shader"))
        {
            m_shader_color.loadFromFile(shaderPathColor, sf::Shader::Fragment);
            m_shader_heat.loadFromFile(shaderPathHeat, sf::Shader::Fragment);
        }

        //ImGui::Checkbox("Draw Contour Lines", &m_drawContours);
        ImGui::SliderInt("Contour Lines", &m_numberOfContourLines, 0, 19);

        //ImGui::Spacing();

    }

    ImGui::Separator();

    // Set algorithm used for computations
    ImGui::Combo("Algorithm",
        (int*)&m_heatGrid.m_algorithm,
        AlgorithmNames,
        IM_ARRAYSIZE(AlgorithmNames));

    // Move time forwards, or reset it
    {

        ImGui::SliderInt("Iterations Per Frame", &m_iterations, 0, 500);
        

        if (ImGui::Button("Step"))
        {
            m_doStep = true;
        }   ImGui::SameLine();

        if (ImGui::Button("Reset"))
        {
            m_iterations = 0;
            m_heatGrid.reset();
        }
    }

    ImGui::Separator();

    {
        std::vector<std::string> sourceStrings; 
        sourceStrings.reserve(m_heatGrid.getSources().size());
        std::vector<const char*> sourceCStrings; 
        sourceCStrings.reserve(m_heatGrid.getSources().size());
        
        for (size_t s = 0; s < m_heatGrid.getSources().size(); s++)
        {
            auto& source = m_heatGrid.getSources()[s];
            std::stringstream ss;
            ss << source.m_temp << " : (" << source.m_area.x << ", " << source.m_area.y << ")";
            sourceStrings.push_back(ss.str());
            sourceCStrings.push_back(sourceStrings.back().c_str());
            //std::string label = "S" + std::to_string(s);
            //ImGui::SliderInt2(label.c_str(), &source.m_area.x, 0, 1000);
        }

        // Now use sourceCStrings for the ImGui::Combo function
        ImGui::Combo("Source", &m_selectedSource, sourceCStrings.data(), sourceCStrings.size());

    }

    // Add & delete sources
    {
        static int pos[2]{};
        static int size[2]{};
        static float temp;

        if (ImGui::Button("Add Source##AddSourceButton"))
        {
            pos[0] = 10;
            pos[1] = 10;
            size[0] = 20;
            size[1] = 20;
            temp = 50.f;
            ImGui::OpenPopup("Add Source##AddSourceModal");
        }

        if (ImGui::BeginPopupModal("Add Source##AddSourceModal")) {
            ImGui::InputInt2("Position", pos);
            ImGui::InputInt2("Size", size);
            ImGui::SliderFloat("Temp", &temp, 0.f, 100.f);

            if (ImGui::Button("Add"))
            {
                //m_heatGrid.addSource({ { pos[0], pos[1] }, { size[0], size[1] }, temp });
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        if (ImGui::Button("Draw Source"))
        {
            m_drawingSource = true;
        }

        if (ImGui::Button("Clear Sources"))
        {
            m_heatGrid.clearSources();
        }
    }
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
    
    const bool draggingProjection = m_projector.processEvent(event, mouse);

    sf::Vector2f ms = mouse;
    ms /= m_projector.getTransformedScale();
    ms -= m_projector.getTransformedPosition();
    cv::Mat projectedMat = (cv::Mat_<double>(1, 3) << ms.x, ms.y, 1.f) * m_projector.getProjectionMatrix().inv();
    sf::Vector2f mousePos = sf::Vector2f{ (float)projectedMat.at<double>(0, 0), (float)projectedMat.at<double>(0, 1) };
    cv::Point mousePoint{ (int)mousePos.y, (int)mousePos.x };
    //cv::Point mousePoint((int)ms.y, (int)ms.x);


    if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
    {
        sf::Vector2f diff = mouse - m_previousMouse;

        if (diff.x != 0 || diff.y != 0)
        {
            m_heatGrid.getSources()[m_selectedSource].m_area.x += diff.x;
            m_heatGrid.getSources()[m_selectedSource].m_area.y += diff.y;
            m_heatGrid.updateSources();
        }
    }

    m_previousMouse = mouse;
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
        m_heatGrid.update(data, m_iterations);

        if (m_doStep)
        {
            m_heatGrid.update(data, 1);
            m_doStep = false;
        }

        {
            PROFILE_SCOPE("Calibration TransformProjection");
            m_projector.project(m_heatGrid.normalizedData(), m_cvTransformedDepthImage32fHeat);
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
