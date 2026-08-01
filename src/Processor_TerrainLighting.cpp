#include "Processor_TerrainLighting.h"
#include "Profiler.hpp"
#include "Tools.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr const char * TerrainLightingShaderPath = "shaders/shader_terrain_lighting.frag";
    constexpr float Pi = 3.14159265358979323846f;
}

void Processor_TerrainLighting::init()
{
    reloadShader();
}

void Processor_TerrainLighting::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(TerrainLightingShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Processor_TerrainLighting::imgui()
{
    PROFILE_FUNCTION();

    const char * palettes[] = { "Terrain", "Grayscale", "Desert", "Ice" };
    ImGui::Combo("Palette", &m_palette, palettes, IM_ARRAYSIZE(palettes));
    ImGui::SliderFloat("Light Azimuth", &m_lightAzimuth, 0.0f, 360.0f, "%.0f deg");
    ImGui::SliderFloat("Light Elevation", &m_lightElevation, 1.0f, 89.0f, "%.0f deg");
    ImGui::TextUnformatted("Left mouse: move light");
    ImGui::SliderFloat("Ambient Light", &m_ambientLight, 0.0f, 1.0f);
    ImGui::SliderFloat("Shadow Strength", &m_shadowStrength, 0.0f, 2.0f);
    ImGui::SliderFloat("Height Strength", &m_heightStrength, 0.1f, 30.0f);

    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }

}

void Processor_TerrainLighting::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (m_hasFrame)
    {
        m_sprite.setPosition(projector().getTransformedPosition());
        const float scale = projector().getTransformedScale();
        m_sprite.setScale({ scale, scale });

        if (m_shaderLoaded)
        {
            m_shader.setUniform("lightAzimuth", m_lightAzimuth);
            m_shader.setUniform("lightElevation", m_lightElevation);
            m_shader.setUniform("ambientLight", m_ambientLight);
            m_shader.setUniform("shadowStrength", m_shadowStrength);
            m_shader.setUniform("heightStrength", m_heightStrength);
            m_shader.setUniform("palette", m_palette);
            const sf::Vector2u textureSize = m_texture.getSize();
            m_shader.setUniform("texelSize", sf::Glsl::Vec2(
                1.0f / textureSize.x,
                1.0f / textureSize.y));
            window.draw(m_sprite, &m_shader);
        }
        else
        {
            window.draw(m_sprite);
        }
    }

}

void Processor_TerrainLighting::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    const bool draggingProjection = projector().processEvent(event, mouse);

    if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>();
        mouseReleased && mouseReleased->button == sf::Mouse::Button::Left)
    {
        m_draggingLight = false;
        return;
    }

    if (draggingProjection || ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>();
        mousePressed && mousePressed->button == sf::Mouse::Button::Left)
    {
        m_draggingLight = updateLightFromMouse(mouse);
    }
    else if (event.is<sf::Event::MouseMoved>() && m_draggingLight)
    {
        if (!updateLightFromMouse(mouse))
        {
            m_draggingLight = false;
        }
    }
}

bool Processor_TerrainLighting::updateLightFromMouse(const sf::Vector2f & mouse)
{
    if (!m_hasFrame)
    {
        return false;
    }

    const float scale = projector().getTransformedScale();
    const sf::Vector2u textureSize = m_texture.getSize();
    if (!std::isfinite(scale) || scale <= 0.0f || textureSize.x == 0 || textureSize.y == 0)
    {
        return false;
    }

    const sf::Vector2f local = (mouse - projector().getTransformedPosition()) / scale;
    if (local.x < 0.0f || local.x >= textureSize.x || local.y < 0.0f || local.y >= textureSize.y)
    {
        return false;
    }

    const float dx = local.x - textureSize.x * 0.5f;
    const float dy = local.y - textureSize.y * 0.5f;
    const float distance = std::sqrt(dx * dx + dy * dy);
    const float radius = std::max(std::min(textureSize.x, textureSize.y) * 0.5f, 1.0f);

    if (distance > 0.001f)
    {
        m_lightAzimuth = std::atan2(dy, dx) * 180.0f / Pi;
        if (m_lightAzimuth < 0.0f)
        {
            m_lightAzimuth += 360.0f;
        }
    }

    const float normalizedDistance = std::clamp(distance / radius, 0.0f, 1.0f);
    m_lightElevation = 89.0f - normalizedDistance * 88.0f;
    return true;
}

void Processor_TerrainLighting::save(Save & save) const
{
    Save::Json & settings = save.section("Processor_TerrainLighting");
    settings["m_palette"] = m_palette;
    settings["m_lightAzimuth"] = m_lightAzimuth;
    settings["m_lightElevation"] = m_lightElevation;
    settings["m_ambientLight"] = m_ambientLight;
    settings["m_shadowStrength"] = m_shadowStrength;
    settings["m_heightStrength"] = m_heightStrength;
}

void Processor_TerrainLighting::load(const Save & save)
{
    const Save::Json & settings = save.section("Processor_TerrainLighting");
    Save::read(settings, "m_palette", m_palette);
    Save::read(settings, "m_lightAzimuth", m_lightAzimuth);
    Save::read(settings, "m_lightElevation", m_lightElevation);
    Save::read(settings, "m_ambientLight", m_ambientLight);
    Save::read(settings, "m_shadowStrength", m_shadowStrength);
    Save::read(settings, "m_heightStrength", m_heightStrength);
}

void Processor_TerrainLighting::processTopography(const IntermediateData & data)
{
    PROFILE_FUNCTION();

    m_hasFrame = Tools::updateProjectedTexture(
        data.topography,
        projector(),
        m_projectedTopography,
        m_image,
        m_texture,
        m_sprite,
        true,
        "Failed to load the terrain-lighting texture.\n");
}
