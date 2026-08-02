#include "Source_Waves.h"
#include "Tools.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;
    constexpr float MinimumDrawableHeight = 6.0f / 255.0f;
    constexpr float MaximumDrawableHeight = 252.0f / 255.0f;
    constexpr int RadialFalloffSteps = 1024;

    const std::array<float, RadialFalloffSteps + 1> RadialFalloff = []()
    {
        std::array<float, RadialFalloffSteps + 1> falloff{};
        for (int i = 0; i <= RadialFalloffSteps; i++)
        {
            const float normalizedRadius = std::sqrt((float)i / RadialFalloffSteps);
            falloff[i] = 0.5f + 0.5f * std::cos(Pi * normalizedRadius);
        }
        return falloff;
    }();
}

void Source_Waves::init()
{
    m_topography = cv::Mat(cv::Size(CanvasSize, CanvasSize), CV_32F, cv::Scalar(0.5f));
    m_clock.restart();
    updateTopography();
}

void Source_Waves::updateTopography()
{
    if (m_topography.empty())
    {
        m_topography = cv::Mat(cv::Size(CanvasSize, CanvasSize), CV_32F, cv::Scalar(0.5f));
    }
    const float elapsedSeconds = std::min(m_clock.restart().asSeconds(), 0.25f);
    m_phase = std::fmod(m_phase + elapsedSeconds * m_frequency * 2.0f * Pi, 2.0f * Pi);
    const bool traveling = m_animationMode == 0;
    const float separation = std::max(m_separation, 1.0f);
    const float centerOffset = separation * 0.5f;
    const float travelOffset = traveling ? (m_phase / (2.0f * Pi)) * separation : 0.0f;
    const float radiusSquared = m_waveSize * m_waveSize;
    const float heightOscillation = std::sin(m_phase);

    std::array<float, CanvasSize> horizontalDistances;
    std::array<float, CanvasSize> verticalDistances;
    std::array<int, CanvasSize> horizontalCells;
    std::array<int, CanvasSize> verticalCells;
    for (int coordinate = 0; coordinate < CanvasSize; coordinate++)
    {
        const float shifted = coordinate - centerOffset - travelOffset;
        const int nearestCell = (int)std::floor(shifted / separation + 0.5f);
        const float distance = shifted - nearestCell * separation;
        horizontalDistances[coordinate] = distance;
        verticalDistances[coordinate] = distance;
        horizontalCells[coordinate] = nearestCell;
        verticalCells[coordinate] = nearestCell;
    }

    for (int y = 0; y < CanvasSize; y++)
    {
        float * row = m_topography.ptr<float>(y);
        const float dy = verticalDistances[y];
        for (int x = 0; x < CanvasSize; x++)
        {
            const float dx = horizontalDistances[x];
            const float distanceSquared = dx * dx + dy * dy;
            float influence = 0.0f;
            if (distanceSquared < radiusSquared)
            {
                const float normalizedDistanceSquared = distanceSquared / radiusSquared;
                const int falloffIndex = std::clamp(
                    (int)(normalizedDistanceSquared * RadialFalloffSteps),
                    0,
                    RadialFalloffSteps);
                influence = RadialFalloff[falloffIndex];
            }

            float displacement = heightOscillation;
            if (traveling)
            {
                const int parity = (horizontalCells[x] + verticalCells[y]) & 1;
                displacement = parity == 0 ? 1.0f : -1.0f;
            }
            row[x] = std::clamp(
                0.5f + influence * displacement * m_amplitude,
                MinimumDrawableHeight,
                MaximumDrawableHeight);
        }
    }

    m_image = Tools::MatToSfImage(m_topography);
    m_textureDirty = true;
    markTerrainChanged();
}

void Source_Waves::updateTexture()
{
    if (!m_textureDirty)
    {
        return;
    }

    if (!m_texture.loadFromImage(m_image))
    {
        std::cerr << "Failed to load the waves terrain texture.\n";
        return;
    }
    m_texture.setSmooth(true);
    m_sprite.setTexture(m_texture, true);
    m_textureDirty = false;
}

void Source_Waves::imgui()
{
    const char * animationModes[] = { "Traveling", "Fixed Position Height Pulse" };
    ImGui::Combo("Animation Mode", &m_animationMode, animationModes, IM_ARRAYSIZE(animationModes));
    ImGui::SliderFloat("Frequency", &m_frequency, 0.0f, 3.0f, "%.2f cycles/s");
    ImGui::SliderFloat("Amplitude", &m_amplitude, 0.0f, 0.475f, "%.3f");
    if (ImGui::SliderFloat("Wave Separation", &m_separation, 8.0f, 256.0f, "%.0f px"))
    {
        m_waveSize = std::min(m_waveSize, m_separation * 0.5f);
    }
    const float maximumRadius = std::max(1.0f, m_separation * 0.5f);
    ImGui::SliderFloat("Wave Size", &m_waveSize, 1.0f, maximumRadius, "%.0f px radius");
    if (m_animationMode == 0)
    {
        ImGui::TextUnformatted("Wave peaks travel across the terrain.");
    }
    else
    {
        ImGui::TextUnformatted("Wave positions stay fixed; only their height oscillates.");
    }
}

void Source_Waves::render(sf::RenderWindow & window)
{
    updateTexture();
    window.draw(m_sprite);
}

void Source_Waves::save(Settings & save) const
{
    Settings::json & settings = save.section("Source_Waves");
    settings["m_animationMode"] = m_animationMode;
    settings["m_frequency"] = m_frequency;
    settings["m_amplitude"] = m_amplitude;
    settings["m_separation"] = m_separation;
    settings["m_waveSize"] = m_waveSize;
}

void Source_Waves::load(const Settings & save)
{
    const Settings::json & settings = save.section("Source_Waves");
    Settings::read(settings, "m_animationMode", m_animationMode);
    Settings::read(settings, "m_frequency", m_frequency);
    Settings::read(settings, "m_amplitude", m_amplitude);
    Settings::read(settings, "m_separation", m_separation);
    Settings::read(settings, "m_waveSize", m_waveSize);
    m_animationMode = std::clamp(m_animationMode, 0, 1);
    m_frequency = std::clamp(m_frequency, 0.0f, 3.0f);
    m_amplitude = std::clamp(m_amplitude, 0.0f, 0.475f);
    m_separation = std::clamp(m_separation, 8.0f, 256.0f);
    m_waveSize = std::clamp(m_waveSize, 1.0f, m_separation * 0.5f);
    m_clock.restart();
}

cv::Mat Source_Waves::getTopography()
{
    updateTopography();
    return m_topography;
}
