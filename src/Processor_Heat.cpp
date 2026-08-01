#include "Processor_Heat.h"
#include "Profiler.hpp"
#include "Tools.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include <iostream>

namespace {
    const std::string shaderPathColor = "shaders/shader_contour_color.frag";
    const std::string shaderPathHeat = "shaders/shader_heat.frag";
}

void Processor_Heat::init()
{
    setInitialHeatSources();
    if (!m_shader_color.loadFromFile(shaderPathColor, sf::Shader::Type::Fragment))
    {
        std::cerr << "Failed to load the heat color shader.\n";
    }
    if (!m_shader_heat.loadFromFile(shaderPathHeat, sf::Shader::Type::Fragment))
    {
        std::cerr << "Failed to load the heat shader.\n";
    }
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

    // Set algorithm used for computations
    ImGui::Combo("Algorithm", (int*)&m_heatGrid.m_algorithm, AlgorithmNames.data(), (int)AlgorithmNames.size());
    ImGui::SliderInt("Iterations Per Frame", &m_iterations, 0, 200);
        

    if (ImGui::Button("Step")) 
    {
        m_doStep = true;
    }   ImGui::SameLine();

    if (ImGui::Button("Reset"))
    {
        m_iterations = 0;
        m_heatGrid.reset();
    }
    
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
    }

    // Now use sourceCStrings for the ImGui::Combo function
    ImGui::Combo("Source", &m_selectedSource, sourceCStrings.data(), (int)sourceCStrings.size());

    if (ImGui::Button("Clear Sources"))
    {
        m_heatGrid.clearSources();
        m_selectedSource = 0;
    }

    ImGui::Separator();

    if (ImGui::Button("Reload Shader"))
    {
        if (!m_shader_color.loadFromFile(shaderPathColor, sf::Shader::Type::Fragment))
        {
            std::cerr << "Failed to reload the heat color shader.\n";
        }
        if (!m_shader_heat.loadFromFile(shaderPathHeat, sf::Shader::Type::Fragment))
        {
            std::cerr << "Failed to reload the heat shader.\n";
        }
    }

    ImGui::Checkbox("##Contours", &m_drawContours);
    ImGui::SameLine();
    ImGui::SliderInt("Contour Lines", &m_numberOfContourLines, 0, 19);


}

void Processor_Heat::render(sf::RenderWindow& window)
{
    PROFILE_FUNCTION();
    if (m_drawProjection)
    {
        PROFILE_SCOPE("Draw Transformed Image");

        {
            m_sfTransformedDepthSpriteColor.setPosition(projector().getTransformedPosition());
            float scale = projector().getTransformedScale();
            m_sfTransformedDepthSpriteColor.setScale({ scale, scale });

            static sf::Clock time;

            //Change color scheme
            m_shader_color.setUniform("shaderIndex", 1);
            m_shader_color.setUniform("contour", m_drawContours);
            m_shader_color.setUniform("numberOfContourLines", m_numberOfContourLines);
            m_shader_color.setUniform("u_time", time.getElapsedTime().asSeconds());
        }
        
        {
            m_sfTransformedDepthSpriteHeat.setPosition(projector().getTransformedPosition());
            float scale = projector().getTransformedScale();
            m_sfTransformedDepthSpriteHeat.setScale({ scale, scale });

            //Change color scheme
            m_shader_heat.setUniform("contour", m_drawContours);
            m_shader_heat.setUniform("numberOfContourLines", m_numberOfContourLines);

            window.draw(m_sfTransformedDepthSpriteHeat, &m_shader_heat);
        }
    }

}

void Processor_Heat::processEvent(const sf::Event& event, const sf::Vector2f& mouse)
{
    PROFILE_FUNCTION();
    
    projector().processEvent(event, mouse);

    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) &&
        m_selectedSource >= 0 && m_selectedSource < (int)m_heatGrid.getSources().size())
    {
        sf::Vector2f diff = mouse - m_previousMouse;

        if (diff.x != 0 || diff.y != 0)
        {
            m_heatGrid.getSources()[m_selectedSource].m_area.x += (int)diff.x;
            m_heatGrid.getSources()[m_selectedSource].m_area.y += (int)diff.y;
            m_heatGrid.updateSources();
        }
    }

    m_previousMouse = mouse;
}

void Processor_Heat::save(Settings& save) const
{
    Settings::json & settings = save.section("Processor_Heat");
    settings["m_drawContours"] = m_drawContours;
    settings["m_numberOfContourLines"] = m_numberOfContourLines;
    settings["m_drawProjection"] = m_drawProjection;
    settings["m_iterations"] = m_iterations;
    settings["m_selectedSource"] = m_selectedSource;
    settings["m_algorithm"] = (int)m_heatGrid.m_algorithm;
    settings["m_sources"] = Settings::json::array();
    for (const HeatSource & source : m_heatGrid.getSources())
    {
        settings["m_sources"].push_back({
            { "m_temp", source.m_temp },
            { "m_area", {
                { "x", source.m_area.x },
                { "y", source.m_area.y },
                { "width", source.m_area.width },
                { "height", source.m_area.height }
            } }
        });
    }
}
void Processor_Heat::load(const Settings& save)
{
    const Settings::json & settings = save.section("Processor_Heat");
    Settings::read(settings, "m_drawContours", m_drawContours);
    Settings::read(settings, "m_numberOfContourLines", m_numberOfContourLines);
    Settings::read(settings, "m_drawProjection", m_drawProjection);
    Settings::read(settings, "m_iterations", m_iterations);
    Settings::read(settings, "m_selectedSource", m_selectedSource);
    int algorithm = (int)m_heatGrid.m_algorithm;
    Settings::read(settings, "m_algorithm", algorithm);
    m_heatGrid.m_algorithm = (Algorithms)algorithm;
    const auto sources = settings.find("m_sources");
    if (sources != settings.end() && sources->is_array())
    {
        m_heatGrid.clearSources();
        for (const Settings::json & sourceSettings : *sources)
        {
            const Settings::json & areaSettings = sourceSettings.at("m_area");
            const cv::Rect area(
                areaSettings.at("x").get<int>(),
                areaSettings.at("y").get<int>(),
                areaSettings.at("width").get<int>(),
                areaSettings.at("height").get<int>());
            m_heatGrid.addSource(HeatSource(area, sourceSettings.at("m_temp").get<float>()));
        }
    }
}

void Processor_Heat::processTopography(const IntermediateData& data)
{
    PROFILE_FUNCTION();

    {
        PROFILE_SCOPE("Color");

        {
            PROFILE_SCOPE("Calibration TransformProjection");
            projector().project(data.topography, m_cvTransformedDepthImage32fColor);
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
                    if (!m_sfTransformedDepthTextureColor.loadFromImage(m_sfTransformedDepthImageColor))
                    {
                        std::cerr << "Failed to load the heat color texture.\n";
                    }
                    else
                    {
                        m_sfTransformedDepthSpriteColor.setTexture(m_sfTransformedDepthTextureColor, true);
                    }
                }
            }
        }
    }

    {
        PROFILE_SCOPE("Heat");
        m_heatGrid.update(data.topography, m_iterations);

        if (m_doStep)
        {
            m_heatGrid.update(data.topography, 1);
            m_doStep = false;
        }

        {
            PROFILE_SCOPE("Calibration TransformProjection");
            projector().project(m_heatGrid.normalizedData(), m_cvTransformedDepthImage32fHeat);
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
                    if (!m_sfTransformedDepthTextureHeat.loadFromImage(m_sfTransformedDepthImageHeat))
                    {
                        std::cerr << "Failed to load the heat-map texture.\n";
                    }
                    else
                    {
                        m_sfTransformedDepthSpriteHeat.setTexture(m_sfTransformedDepthTextureHeat, true);
                    }
                }
            }
        }
    }
}
