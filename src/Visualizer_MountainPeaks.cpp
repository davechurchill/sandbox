#include "Visualizer_MountainPeaks.h"
#include "Profiler.hpp"

#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr const char * MountainPeaksShaderPath = "shaders/shader_mountain_peaks.frag";
    constexpr float DefaultSnowLine = 0.58f;
    constexpr float DefaultSnowCoverage = 1.0f;
    constexpr float DefaultRockContrast = 1.2f;
    constexpr float DefaultSunAzimuth = 315.0f;
    constexpr float DefaultSunElevation = 38.0f;
    constexpr float DefaultHaze = 0.32f;
}

void Visualizer_MountainPeaks::init()
{
    reloadShader();
}

void Visualizer_MountainPeaks::resetDefaults()
{
    m_time = 0.0f;
    m_snowLine = DefaultSnowLine;
    m_snowCoverage = DefaultSnowCoverage;
    m_rockContrast = DefaultRockContrast;
    m_sunAzimuth = DefaultSunAzimuth;
    m_sunElevation = DefaultSunElevation;
    m_haze = DefaultHaze;
}

void Visualizer_MountainPeaks::clampSettings()
{
    m_snowLine = std::isfinite(m_snowLine) ? std::clamp(m_snowLine, 0.10f, 0.90f) : DefaultSnowLine;
    m_snowCoverage = std::isfinite(m_snowCoverage) ? std::clamp(m_snowCoverage, 0.0f, 2.0f) : DefaultSnowCoverage;
    m_rockContrast = std::isfinite(m_rockContrast) ? std::clamp(m_rockContrast, 0.25f, 2.5f) : DefaultRockContrast;
    m_sunAzimuth = std::isfinite(m_sunAzimuth) ? std::clamp(m_sunAzimuth, 0.0f, 360.0f) : DefaultSunAzimuth;
    m_sunElevation = std::isfinite(m_sunElevation) ? std::clamp(m_sunElevation, 5.0f, 85.0f) : DefaultSunElevation;
    m_haze = std::isfinite(m_haze) ? std::clamp(m_haze, 0.0f, 1.5f) : DefaultHaze;
}

void Visualizer_MountainPeaks::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(MountainPeaksShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Visualizer_MountainPeaks::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderFloat("Snow Line", &m_snowLine, 0.10f, 0.90f);
    ImGui::SliderFloat("Snow Coverage", &m_snowCoverage, 0.0f, 2.0f);
    ImGui::SliderFloat("Rock Contrast", &m_rockContrast, 0.25f, 2.5f);
    ImGui::SliderFloat("Sun Azimuth", &m_sunAzimuth, 0.0f, 360.0f, "%.0f deg");
    ImGui::SliderFloat("Sun Elevation", &m_sunElevation, 5.0f, 85.0f, "%.0f deg");
    ImGui::SliderFloat("Haze", &m_haze, 0.0f, 1.5f);

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

void Visualizer_MountainPeaks::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (!m_hasFrame)
    {
        return;
    }

    const sf::Vector2u textureSize = m_texture.getSize();
    if (textureSize.x == 0 || textureSize.y == 0)
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

    m_shader.setUniform("texelSize", sf::Glsl::Vec2(1.0f / textureSize.x, 1.0f / textureSize.y));
    m_shader.setUniform("u_time", m_time);
    m_shader.setUniform("snowLine", m_snowLine);
    m_shader.setUniform("snowCoverage", m_snowCoverage);
    m_shader.setUniform("rockContrast", m_rockContrast);
    m_shader.setUniform("sunAzimuth", m_sunAzimuth);
    m_shader.setUniform("sunElevation", m_sunElevation);
    m_shader.setUniform("hazeAmount", m_haze);
    window.draw(m_sprite, &m_shader);
}

void Visualizer_MountainPeaks::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    projector().processEvent(event, mouse);
}

void Visualizer_MountainPeaks::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_MountainPeaks");
    settings["m_snowLine"] = m_snowLine;
    settings["m_snowCoverage"] = m_snowCoverage;
    settings["m_rockContrast"] = m_rockContrast;
    settings["m_sunAzimuth"] = m_sunAzimuth;
    settings["m_sunElevation"] = m_sunElevation;
    settings["m_haze"] = m_haze;
}

void Visualizer_MountainPeaks::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_MountainPeaks");
    Settings::read(settings, "m_snowLine", m_snowLine);
    Settings::read(settings, "m_snowCoverage", m_snowCoverage);
    Settings::read(settings, "m_rockContrast", m_rockContrast);
    Settings::read(settings, "m_sunAzimuth", m_sunAzimuth);
    Settings::read(settings, "m_sunElevation", m_sunElevation);
    Settings::read(settings, "m_haze", m_haze);
    clampSettings();
}

void Visualizer_MountainPeaks::process(const TerrainFrame & data)
{
    PROFILE_FUNCTION();

    const float deltaTime = std::isfinite(data.deltaTime) ? std::clamp(data.deltaTime, 0.0f, 0.10f) : 0.0f;
    m_time = std::fmod(m_time + deltaTime, 4096.0f);
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
        "Failed to load the Mountain Peaks terrain texture.\n");
}
