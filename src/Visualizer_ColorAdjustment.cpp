#include "Visualizer_ColorAdjustment.h"

#include "Profiler.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
    constexpr const char * ColorAdjustmentShaderPath =
        "shaders/shader_color_adjustment.frag";
}

void Visualizer_ColorAdjustment::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(
        ColorAdjustmentShaderPath,
        sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
    else
    {
        std::cerr << "Failed to load the color-adjustment overlay shader.\n";
    }
}

void Visualizer_ColorAdjustment::resetAdjustments()
{
    m_brightness = 0.0f;
    m_contrast = 1.0f;
    m_exposure = 0.0f;
    m_saturation = 1.0f;
    m_hue = 0.0f;
    m_gamma = 1.0f;
    m_temperature = 0.0f;
}

bool Visualizer_ColorAdjustment::hasAdjustments() const
{
    constexpr float Epsilon = 0.0001f;
    return std::abs(m_brightness) > Epsilon
        || std::abs(m_contrast - 1.0f) > Epsilon
        || std::abs(m_exposure) > Epsilon
        || std::abs(m_saturation - 1.0f) > Epsilon
        || std::abs(m_hue) > Epsilon
        || std::abs(m_gamma - 1.0f) > Epsilon
        || std::abs(m_temperature) > Epsilon;
}

void Visualizer_ColorAdjustment::init()
{
    reloadShader();
}

void Visualizer_ColorAdjustment::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderFloat("Brightness", &m_brightness, -1.0f, 1.0f, "%+.2f");
    ImGui::SliderFloat("Contrast", &m_contrast, 0.0f, 2.0f, "%.2fx");
    ImGui::SliderFloat("Exposure", &m_exposure, -3.0f, 3.0f, "%+.2f EV");
    ImGui::SliderFloat("Saturation", &m_saturation, 0.0f, 2.0f, "%.2fx");
    ImGui::SliderFloat("Hue", &m_hue, -180.0f, 180.0f, "%+.0f deg");
    ImGui::SliderFloat("Gamma", &m_gamma, 0.20f, 3.0f, "%.2f");
    ImGui::SliderFloat("Temperature", &m_temperature, -1.0f, 1.0f, "%+.2f");

    if (ImGui::Button("Reset Adjustments"))
    {
        resetAdjustments();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }
}

void Visualizer_ColorAdjustment::process(
    const TerrainFrame & data)
{
    PROFILE_FUNCTION();

    if (!data.heightMap.empty())
    {
        m_hasFrame = context().projector().updateTexture(
            data.heightMap,
            m_projectedTopography,
            m_terrainImage,
            m_terrainTexture,
            m_terrainSprite,
            false,
            "Failed to load the color-adjustment terrain mask.\n");
    }
}

void Visualizer_ColorAdjustment::render(
    sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (!m_hasFrame || !m_shaderLoaded || !hasAdjustments())
    {
        return;
    }

    const sf::Vector2u windowSize = window.getSize();
    const sf::Vector2u terrainSize = m_terrainTexture.getSize();
    const float scale = context().projector().getTransformedScale();
    const sf::Vector2f origin = context().projector().getTransformedPosition();
    if (windowSize.x == 0 || windowSize.y == 0
        || terrainSize.x == 0 || terrainSize.y == 0
        || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    if (m_captureTexture.getSize() != windowSize)
    {
        if (!m_captureTexture.resize(windowSize))
        {
            std::cerr << "Failed to resize the color-adjustment capture texture.\n";
            return;
        }
        m_captureSprite.setTexture(m_captureTexture, true);
    }

    const sf::Vector2f projectedSize(
        terrainSize.x * scale,
        terrainSize.y * scale);
    const int left = std::max(0, (int)std::floor(origin.x));
    const int top = std::max(0, (int)std::floor(origin.y));
    const int right = std::min(
        (int)windowSize.x,
        (int)std::ceil(origin.x + projectedSize.x));
    const int bottom = std::min(
        (int)windowSize.y,
        (int)std::ceil(origin.y + projectedSize.y));
    if (right <= left || bottom <= top)
    {
        return;
    }

    m_captureTexture.update(window);
    m_captureSprite.setTextureRect({
        { left, top },
        { right - left, bottom - top } });
    m_captureSprite.setPosition({ (float)left, (float)top });
    m_captureSprite.setScale({ 1.0f, 1.0f });

    m_shader.setUniform("terrainTexture", m_terrainTexture);
    m_shader.setUniform("windowHeight", (float)windowSize.y);
    m_shader.setUniform("projectionOrigin", sf::Glsl::Vec2(origin));
    m_shader.setUniform("projectionSize", sf::Glsl::Vec2(projectedSize));
    m_shader.setUniform("brightness", m_brightness);
    m_shader.setUniform("contrast", m_contrast);
    m_shader.setUniform("exposure", m_exposure);
    m_shader.setUniform("saturation", m_saturation);
    m_shader.setUniform("hue", m_hue);
    m_shader.setUniform("gamma", m_gamma);
    m_shader.setUniform("temperature", m_temperature);
    window.draw(m_captureSprite, &m_shader);
}

void Visualizer_ColorAdjustment::processEvent(
    const sf::Event &,
    const sf::Vector2f &)
{
}

void Visualizer_ColorAdjustment::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_ColorAdjustment");
    settings["m_brightness"] = m_brightness;
    settings["m_contrast"] = m_contrast;
    settings["m_exposure"] = m_exposure;
    settings["m_saturation"] = m_saturation;
    settings["m_hue"] = m_hue;
    settings["m_gamma"] = m_gamma;
    settings["m_temperature"] = m_temperature;
}

void Visualizer_ColorAdjustment::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_ColorAdjustment");
    Settings::read(settings, "m_brightness", m_brightness);
    Settings::read(settings, "m_contrast", m_contrast);
    Settings::read(settings, "m_exposure", m_exposure);
    Settings::read(settings, "m_saturation", m_saturation);
    Settings::read(settings, "m_hue", m_hue);
    Settings::read(settings, "m_gamma", m_gamma);
    Settings::read(settings, "m_temperature", m_temperature);

    m_brightness = std::clamp(m_brightness, -1.0f, 1.0f);
    m_contrast = std::clamp(m_contrast, 0.0f, 2.0f);
    m_exposure = std::clamp(m_exposure, -3.0f, 3.0f);
    m_saturation = std::clamp(m_saturation, 0.0f, 2.0f);
    m_hue = std::clamp(m_hue, -180.0f, 180.0f);
    m_gamma = std::clamp(m_gamma, 0.20f, 3.0f);
    m_temperature = std::clamp(m_temperature, -1.0f, 1.0f);
}
