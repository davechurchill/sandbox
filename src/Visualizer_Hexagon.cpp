#include "Visualizer_Hexagon.h"

#include "Profiler.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr const char * HexagonShaderPath = "shaders/shader_hexagon.frag";
    constexpr int DefaultHexagonSize = 18;
    constexpr int DefaultHeightSteps = 28;
    constexpr float DefaultPrismRelief = 1.15f;
}

void Visualizer_Hexagon::init()
{
    reloadShader();
}

void Visualizer_Hexagon::resetDefaults()
{
    m_hexagonSize = DefaultHexagonSize;
    m_heightSteps = DefaultHeightSteps;
    m_prismRelief = DefaultPrismRelief;
}

void Visualizer_Hexagon::clampSettings()
{
    m_hexagonSize = std::clamp(m_hexagonSize, 6, 48);
    m_heightSteps = std::clamp(m_heightSteps, 8, 64);
    m_prismRelief = std::isfinite(m_prismRelief) ? std::clamp(m_prismRelief, 0.25f, 3.0f) : DefaultPrismRelief;
}

void Visualizer_Hexagon::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(HexagonShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Visualizer_Hexagon::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderInt("Hexagon Size", &m_hexagonSize, 6, 48);
    ImGui::SliderInt("Height Steps", &m_heightSteps, 8, 64);
    ImGui::SliderFloat("Prism Relief", &m_prismRelief, 0.25f, 3.0f);

    if (ImGui::Button("Reset Defaults"))
    {
        resetDefaults();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }
}

void Visualizer_Hexagon::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (!m_shaderLoaded)
    {
        projector().drawTerrain(window);
        return;
    }

    const sf::Texture * terrainTexture = projector().terrainTexture();
    if (!terrainTexture) { return; }
    const sf::Vector2u textureSize = terrainTexture->getSize();

    m_shader.setUniform("texelSize", sf::Glsl::Vec2(1.0f / textureSize.x, 1.0f / textureSize.y));
    m_shader.setUniform("hexagonSize", (float)m_hexagonSize);
    m_shader.setUniform("heightSteps", (float)m_heightSteps);
    m_shader.setUniform("prismRelief", m_prismRelief);
    projector().drawTerrain(window, &m_shader);
}

void Visualizer_Hexagon::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    projector().processEvent(event, mouse);
}

void Visualizer_Hexagon::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_Hexagon");
    settings = Settings::json::object();
    settings["m_hexagonSize"] = m_hexagonSize;
    settings["m_heightSteps"] = m_heightSteps;
    settings["m_prismRelief"] = m_prismRelief;
}

void Visualizer_Hexagon::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_Hexagon");
    Settings::read(settings, "m_hexagonSize", m_hexagonSize);
    Settings::read(settings, "m_heightSteps", m_heightSteps);
    Settings::read(settings, "m_prismRelief", m_prismRelief);
    clampSettings();
}
