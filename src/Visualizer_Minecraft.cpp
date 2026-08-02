#include "Visualizer_Minecraft.h"

#include "Profiler.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr const char * MinecraftShaderPath = "shaders/shader_minecraft.frag";
    constexpr int DefaultBlockSize = 24;
    constexpr int DefaultHeightSteps = 28;
    constexpr float DefaultBlockRelief = 1.0f;
    constexpr float DefaultWaterLevel = 0.28f;
    constexpr float DefaultSnowLine = 0.82f;
    constexpr float DefaultAoStrength = 0.85f;
}

void Visualizer_Minecraft::init()
{
    reloadShader();
}

void Visualizer_Minecraft::resetDefaults()
{
    m_blockSize = DefaultBlockSize;
    m_heightSteps = DefaultHeightSteps;
    m_blockRelief = DefaultBlockRelief;
    m_waterLevel = DefaultWaterLevel;
    m_snowLine = DefaultSnowLine;
    m_aoStrength = DefaultAoStrength;
    m_time = 0.0f;
}

void Visualizer_Minecraft::clampSettings()
{
    m_blockSize = std::clamp(m_blockSize, 8, 64);
    m_heightSteps = std::clamp(m_heightSteps, 8, 64);
    m_blockRelief = std::isfinite(m_blockRelief) ? std::clamp(m_blockRelief, 0.25f, 2.5f) : DefaultBlockRelief;
    m_waterLevel = std::isfinite(m_waterLevel) ? std::clamp(m_waterLevel, 0.05f, 0.60f) : DefaultWaterLevel;
    m_snowLine = std::isfinite(m_snowLine) ? std::clamp(m_snowLine, 0.45f, 0.98f) : DefaultSnowLine;
    m_aoStrength = std::isfinite(m_aoStrength) ? std::clamp(m_aoStrength, 0.0f, 1.5f) : DefaultAoStrength;
}

void Visualizer_Minecraft::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(MinecraftShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Visualizer_Minecraft::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderInt("Block Size", &m_blockSize, 8, 64);
    ImGui::SliderInt("Height Steps", &m_heightSteps, 8, 64);
    ImGui::SliderFloat("Block Relief", &m_blockRelief, 0.25f, 2.5f);
    ImGui::SliderFloat("Water Level", &m_waterLevel, 0.05f, 0.60f);
    ImGui::SliderFloat("Snow Line", &m_snowLine, 0.45f, 0.98f);
    ImGui::SliderFloat("AO / Shadow Strength", &m_aoStrength, 0.0f, 1.5f);

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

void Visualizer_Minecraft::process(const TerrainFrame & data)
{
    PROFILE_FUNCTION();

    const float deltaTime = std::isfinite(data.deltaTime) ? std::clamp(data.deltaTime, 0.0f, 0.10f) : 0.0f;
    m_time = std::fmod(m_time + deltaTime, 4096.0f);
}

void Visualizer_Minecraft::render(sf::RenderWindow & window)
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
    m_shader.setUniform("time", m_time);
    m_shader.setUniform("blockSize", (float)m_blockSize);
    m_shader.setUniform("heightSteps", (float)m_heightSteps);
    m_shader.setUniform("blockRelief", m_blockRelief);
    m_shader.setUniform("waterLevel", m_waterLevel);
    m_shader.setUniform("snowLine", m_snowLine);
    m_shader.setUniform("aoStrength", m_aoStrength);
    projector().drawTerrain(window, &m_shader);
}

void Visualizer_Minecraft::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    projector().processEvent(event, mouse);
}

void Visualizer_Minecraft::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_Minecraft");
    settings["m_blockSize"] = m_blockSize;
    settings["m_heightSteps"] = m_heightSteps;
    settings["m_blockRelief"] = m_blockRelief;
    settings["m_waterLevel"] = m_waterLevel;
    settings["m_snowLine"] = m_snowLine;
    settings["m_aoStrength"] = m_aoStrength;
}

void Visualizer_Minecraft::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_Minecraft");
    Settings::read(settings, "m_blockSize", m_blockSize);
    Settings::read(settings, "m_heightSteps", m_heightSteps);
    Settings::read(settings, "m_blockRelief", m_blockRelief);
    Settings::read(settings, "m_waterLevel", m_waterLevel);
    Settings::read(settings, "m_snowLine", m_snowLine);
    Settings::read(settings, "m_aoStrength", m_aoStrength);
    clampSettings();
}
