#include "Visualizer_GravitationalStarfield.h"

#include "Profiler.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
    constexpr const char * GravitationalStarfieldShaderPath =
        "shaders/shader_gravitational_starfield.frag";
}

void Visualizer_GravitationalStarfield::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(
        GravitationalStarfieldShaderPath,
        sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
    else
    {
        std::cerr << "Failed to load the gravitational-starfield shader.\n";
    }
}

void Visualizer_GravitationalStarfield::resetDefaults()
{
    m_lensingStrength = 1.25f;
    m_starDensity = 58.0f;
    m_nebulaIntensity = 0.85f;
    m_ringIntensity = 1.35f;
    m_driftSpeed = 0.22f;
}

void Visualizer_GravitationalStarfield::init()
{
    reloadShader();
}

void Visualizer_GravitationalStarfield::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderFloat("Lensing Strength", &m_lensingStrength, 0.0f, 4.0f, "%.2f");
    ImGui::SliderFloat("Star Density", &m_starDensity, 16.0f, 120.0f, "%.0f");
    ImGui::SliderFloat("Nebula Intensity", &m_nebulaIntensity, 0.0f, 2.5f, "%.2f");
    ImGui::SliderFloat("Accretion Rings", &m_ringIntensity, 0.0f, 3.0f, "%.2f");
    ImGui::SliderFloat("Drift Speed", &m_driftSpeed, 0.0f, 2.0f, "%.2f");
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

void Visualizer_GravitationalStarfield::process(const TerrainFrame & data)
{
    PROFILE_FUNCTION();

    const float deltaTime = std::isfinite(data.deltaTime)
        ? std::clamp(data.deltaTime, 0.0f, 0.1f)
        : 0.0f;
    m_time = std::fmod(m_time + deltaTime, 1000.0f);
}

void Visualizer_GravitationalStarfield::render(sf::RenderWindow & window)
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
        1.0f / textureSize.x,
        1.0f / textureSize.y));
    m_shader.setUniform("time", m_time);
    m_shader.setUniform("lensingStrength", m_lensingStrength);
    m_shader.setUniform("starDensity", m_starDensity);
    m_shader.setUniform("nebulaIntensity", m_nebulaIntensity);
    m_shader.setUniform("ringIntensity", m_ringIntensity);
    m_shader.setUniform("driftSpeed", m_driftSpeed);
    projector().drawTerrain(window, &m_shader, true);
}

void Visualizer_GravitationalStarfield::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_GravitationalStarfield");
    settings["m_lensingStrength"] = m_lensingStrength;
    settings["m_starDensity"] = m_starDensity;
    settings["m_nebulaIntensity"] = m_nebulaIntensity;
    settings["m_ringIntensity"] = m_ringIntensity;
    settings["m_driftSpeed"] = m_driftSpeed;
}

void Visualizer_GravitationalStarfield::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_GravitationalStarfield");
    Settings::read(settings, "m_lensingStrength", m_lensingStrength);
    Settings::read(settings, "m_starDensity", m_starDensity);
    Settings::read(settings, "m_nebulaIntensity", m_nebulaIntensity);
    Settings::read(settings, "m_ringIntensity", m_ringIntensity);
    Settings::read(settings, "m_driftSpeed", m_driftSpeed);

    m_lensingStrength = std::clamp(m_lensingStrength, 0.0f, 4.0f);
    m_starDensity = std::clamp(m_starDensity, 16.0f, 120.0f);
    m_nebulaIntensity = std::clamp(m_nebulaIntensity, 0.0f, 2.5f);
    m_ringIntensity = std::clamp(m_ringIntensity, 0.0f, 3.0f);
    m_driftSpeed = std::clamp(m_driftSpeed, 0.0f, 2.0f);
}
