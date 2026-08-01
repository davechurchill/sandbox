#include "Visualizer_ContourLines.h"

#include "Profiler.hpp"
#include "imgui.h"

#include <algorithm>
#include <iostream>

namespace
{
    constexpr const char * ContourShaderPath = "shaders/shader_contour_overlay.frag";
}

void Visualizer_ContourLines::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(
        ContourShaderPath,
        sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
    else
    {
        std::cerr << "Failed to load the contour overlay shader.\n";
    }
}

void Visualizer_ContourLines::init()
{
    reloadShader();
}

void Visualizer_ContourLines::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderInt("Contour Lines", &m_numberOfContourLines, 0, 64);
    ImGui::ColorEdit3("Line Color", m_lineColor.data());
    ImGui::SliderFloat("Line Opacity", &m_lineOpacity, 0.0f, 1.0f);
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }
}

void Visualizer_ContourLines::process(
    const TerrainFrame & data)
{
    PROFILE_FUNCTION();

    if (!data.heightMap.empty())
    {
        m_hasFrame = context().projector().updateTexture(
                data.heightMap,
                m_projectedTopography,
                m_image,
                m_texture,
                m_sprite,
                false,
                "Failed to load the contour overlay texture.\n");
    }
}

void Visualizer_ContourLines::render(
    sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (!m_hasFrame || !m_shaderLoaded || m_numberOfContourLines <= 0)
    {
        return;
    }

    m_sprite.setPosition(context().projector().getTransformedPosition());
    const float scale = context().projector().getTransformedScale();
    m_sprite.setScale({ scale, scale });

    m_shader.setUniform("numberOfContourLines", m_numberOfContourLines);
    m_shader.setUniform("lineColor", sf::Glsl::Vec3(
        m_lineColor[0],
        m_lineColor[1],
        m_lineColor[2]));
    m_shader.setUniform("lineOpacity", m_lineOpacity);
    window.draw(m_sprite, &m_shader);
}

void Visualizer_ContourLines::processEvent(
    const sf::Event &,
    const sf::Vector2f &)
{
}

void Visualizer_ContourLines::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_ContourLines");
    settings["m_lineColor"] = m_lineColor;
    settings["m_lineOpacity"] = m_lineOpacity;
    settings["m_numberOfContourLines"] = m_numberOfContourLines;
}

void Visualizer_ContourLines::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_ContourLines");
    Settings::read(settings, "m_lineColor", m_lineColor);
    Settings::read(settings, "m_lineOpacity", m_lineOpacity);
    Settings::read(settings, "m_numberOfContourLines", m_numberOfContourLines);

    for (float & channel : m_lineColor)
    {
        channel = std::clamp(channel, 0.0f, 1.0f);
    }
    m_lineOpacity = std::clamp(m_lineOpacity, 0.0f, 1.0f);
    m_numberOfContourLines = std::clamp(m_numberOfContourLines, 0, 64);
}
