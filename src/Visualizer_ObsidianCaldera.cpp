#include "Visualizer_ObsidianCaldera.h"
#include "Profiler.hpp"

#include "imgui.h"

#include <algorithm>

namespace
{
    constexpr const char * ObsidianCalderaShaderPath = "shaders/shader_obsidian_caldera.frag";
}

void Visualizer_ObsidianCaldera::init()
{
    reloadShader();
}

void Visualizer_ObsidianCaldera::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(ObsidianCalderaShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Visualizer_ObsidianCaldera::resetDefaults()
{
    m_lavaLevel = 0.28f;
    m_crackIntensity = 1.25f;
    m_crackScale = 34.0f;
    m_flowSpeed = 0.75f;
    m_cooling = 0.42f;
    m_heatDistortion = 0.35f;
}

void Visualizer_ObsidianCaldera::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderFloat("Lava Level", &m_lavaLevel, 0.02f, 0.75f);
    ImGui::SliderFloat("Crack Intensity", &m_crackIntensity, 0.0f, 2.5f);
    ImGui::SliderFloat("Crack Scale", &m_crackScale, 8.0f, 96.0f, "%.0f px");
    ImGui::SliderFloat("Flow Speed", &m_flowSpeed, 0.0f, 3.0f);
    ImGui::SliderFloat("Cooling", &m_cooling, 0.0f, 1.0f);
    ImGui::SliderFloat("Heat Distortion", &m_heatDistortion, 0.0f, 1.5f);

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

void Visualizer_ObsidianCaldera::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (!m_hasFrame)
    {
        return;
    }

    m_sprite.setPosition(projector().getTransformedPosition());
    const float scale = projector().getTransformedScale();
    m_sprite.setScale({ scale, scale });

    if (!m_shaderLoaded)
    {
        window.draw(m_sprite);
        return;
    }

    const sf::Vector2u textureSize = m_texture.getSize();
    if (textureSize.x == 0 || textureSize.y == 0)
    {
        return;
    }

    m_shader.setUniform("texelSize", sf::Glsl::Vec2(1.0f / textureSize.x, 1.0f / textureSize.y));
    m_shader.setUniform("u_time", m_clock.getElapsedTime().asSeconds());
    m_shader.setUniform("lavaLevel", m_lavaLevel);
    m_shader.setUniform("crackIntensity", m_crackIntensity);
    m_shader.setUniform("crackScale", m_crackScale);
    m_shader.setUniform("flowSpeed", m_flowSpeed);
    m_shader.setUniform("cooling", m_cooling);
    m_shader.setUniform("heatDistortion", m_heatDistortion);
    window.draw(m_sprite, &m_shader);
}

void Visualizer_ObsidianCaldera::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    projector().processEvent(event, mouse);
}

void Visualizer_ObsidianCaldera::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_ObsidianCaldera");
    settings["m_lavaLevel"] = m_lavaLevel;
    settings["m_crackIntensity"] = m_crackIntensity;
    settings["m_crackScale"] = m_crackScale;
    settings["m_flowSpeed"] = m_flowSpeed;
    settings["m_cooling"] = m_cooling;
    settings["m_heatDistortion"] = m_heatDistortion;
}

void Visualizer_ObsidianCaldera::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_ObsidianCaldera");
    Settings::read(settings, "m_lavaLevel", m_lavaLevel);
    Settings::read(settings, "m_crackIntensity", m_crackIntensity);
    Settings::read(settings, "m_crackScale", m_crackScale);
    Settings::read(settings, "m_flowSpeed", m_flowSpeed);
    Settings::read(settings, "m_cooling", m_cooling);
    Settings::read(settings, "m_heatDistortion", m_heatDistortion);

    m_lavaLevel = std::clamp(m_lavaLevel, 0.02f, 0.75f);
    m_crackIntensity = std::clamp(m_crackIntensity, 0.0f, 2.5f);
    m_crackScale = std::clamp(m_crackScale, 8.0f, 96.0f);
    m_flowSpeed = std::clamp(m_flowSpeed, 0.0f, 3.0f);
    m_cooling = std::clamp(m_cooling, 0.0f, 1.0f);
    m_heatDistortion = std::clamp(m_heatDistortion, 0.0f, 1.5f);
}

void Visualizer_ObsidianCaldera::process(const TerrainFrame & data)
{
    PROFILE_FUNCTION();

    if (data.heightMap.empty() || data.heightMap.type() != CV_32F)
    {
        m_hasFrame = false;
        return;
    }

    m_hasFrame = projector().updateTexture(
        data.heightMap,
        m_projectedTopography,
        m_image,
        m_texture,
        m_sprite,
        true,
        "Failed to load the Obsidian Caldera terrain texture.\n");
}
