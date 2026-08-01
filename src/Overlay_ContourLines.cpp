#include "Overlay_ContourLines.h"

#include "Profiler.hpp"
#include "Tools.h"
#include "imgui.h"

#include <algorithm>
#include <iostream>

namespace
{
    constexpr const char * ContourShaderPath = "shaders/shader_contour_overlay.frag";
}

void Overlay_ContourLines::reloadShader()
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

void Overlay_ContourLines::initOverlay()
{
    reloadShader();
}

void Overlay_ContourLines::imguiOverlay()
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

void Overlay_ContourLines::processTopographyOverlay(
    const IntermediateData & data,
    TopographyProcessor & processor)
{
    PROFILE_FUNCTION();

    if (!data.topography.empty())
    {
        m_hasFrame = Tools::updateProjectedTexture(
                data.topography,
                processor.projector(),
                m_projectedTopography,
                m_image,
                m_texture,
                m_sprite,
                false,
                "Failed to load the contour overlay texture.\n");
    }
}

void Overlay_ContourLines::renderOverlay(
    sf::RenderWindow & window,
    TopographyProcessor & processor)
{
    PROFILE_FUNCTION();

    if (!m_hasFrame || !m_shaderLoaded || m_numberOfContourLines <= 0)
    {
        return;
    }

    m_sprite.setPosition(processor.projector().getTransformedPosition());
    const float scale = processor.projector().getTransformedScale();
    m_sprite.setScale({ scale, scale });

    m_shader.setUniform("numberOfContourLines", m_numberOfContourLines);
    m_shader.setUniform("lineColor", sf::Glsl::Vec3(
        m_lineColor[0],
        m_lineColor[1],
        m_lineColor[2]));
    m_shader.setUniform("lineOpacity", m_lineOpacity);
    window.draw(m_sprite, &m_shader);
}

void Overlay_ContourLines::processOverlayEvent(
    const sf::Event &,
    const sf::Vector2f &,
    TopographyProcessor &)
{
}

void Overlay_ContourLines::saveOverlay(Settings & save) const
{
    Settings::json & settings = save.section("Overlay_ContourLines");
    settings["m_lineColor"] = m_lineColor;
    settings["m_lineOpacity"] = m_lineOpacity;
    settings["m_numberOfContourLines"] = m_numberOfContourLines;
}

void Overlay_ContourLines::loadOverlay(const Settings & save)
{
    const Settings::json & settings = save.section("Overlay_ContourLines");
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
