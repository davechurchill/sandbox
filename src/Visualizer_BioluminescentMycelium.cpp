#include "Visualizer_BioluminescentMycelium.h"
#include "Profiler.hpp"

#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr const char * BioluminescentMyceliumShaderPath =
        "shaders/shader_bioluminescent_mycelium.frag";
}

void Visualizer_BioluminescentMycelium::init()
{
    reloadShader();
}

void Visualizer_BioluminescentMycelium::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(
        BioluminescentMyceliumShaderPath,
        sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Visualizer_BioluminescentMycelium::resetDefaults()
{
    m_glowIntensity = 1.35f;
    m_networkScale = 8.0f;
    m_pulseSpeed = 1.0f;
    m_sporeDensity = 0.35f;
}

void Visualizer_BioluminescentMycelium::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderFloat("Glow Intensity", &m_glowIntensity, 0.0f, 3.0f);
    ImGui::SliderFloat("Network Scale", &m_networkScale, 2.0f, 20.0f, "%.1f");
    ImGui::SliderFloat("Pulse Speed", &m_pulseSpeed, 0.0f, 4.0f);
    ImGui::SliderFloat("Spore Density", &m_sporeDensity, 0.0f, 1.0f);

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

void Visualizer_BioluminescentMycelium::process(const TerrainFrame & data)
{
    PROFILE_FUNCTION();

    const float deltaTime = std::isfinite(data.deltaTime)
        ? std::clamp(data.deltaTime, 0.0f, 0.1f)
        : 0.0f;
    m_time = std::fmod(m_time + deltaTime, 4096.0f);
}

void Visualizer_BioluminescentMycelium::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (!m_shaderLoaded)
    {
        projector().drawTerrain(window, nullptr, true);
        return;
    }

    const sf::Texture * terrainTexture = projector().terrainTexture(true);
    if (!terrainTexture) { return; }
    const sf::Vector2u textureSize = terrainTexture->getSize();

    m_shader.setUniform("texelSize", sf::Glsl::Vec2(
        1.0f / (float)textureSize.x,
        1.0f / (float)textureSize.y));
    m_shader.setUniform("time", m_time);
    m_shader.setUniform("glowIntensity", m_glowIntensity);
    m_shader.setUniform("networkScale", m_networkScale);
    m_shader.setUniform("pulseSpeed", m_pulseSpeed);
    m_shader.setUniform("sporeDensity", m_sporeDensity);
    projector().drawTerrain(window, &m_shader, true);
}

void Visualizer_BioluminescentMycelium::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    projector().processEvent(event, mouse);
}

void Visualizer_BioluminescentMycelium::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_BioluminescentMycelium");
    settings["m_glowIntensity"] = m_glowIntensity;
    settings["m_networkScale"] = m_networkScale;
    settings["m_pulseSpeed"] = m_pulseSpeed;
    settings["m_sporeDensity"] = m_sporeDensity;
}

void Visualizer_BioluminescentMycelium::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_BioluminescentMycelium");
    Settings::read(settings, "m_glowIntensity", m_glowIntensity);
    Settings::read(settings, "m_networkScale", m_networkScale);
    Settings::read(settings, "m_pulseSpeed", m_pulseSpeed);
    Settings::read(settings, "m_sporeDensity", m_sporeDensity);

    m_glowIntensity = std::clamp(m_glowIntensity, 0.0f, 3.0f);
    m_networkScale = std::clamp(m_networkScale, 2.0f, 20.0f);
    m_pulseSpeed = std::clamp(m_pulseSpeed, 0.0f, 4.0f);
    m_sporeDensity = std::clamp(m_sporeDensity, 0.0f, 1.0f);
}
