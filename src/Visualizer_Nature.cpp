#include "Visualizer_Nature.h"
#include "Profiler.hpp"

#include "imgui.h"

namespace
{
    constexpr const char * NatureShaderPath = "shaders/shader_nature.frag";
}

void Visualizer_Nature::init()
{
    reloadShader();
}

void Visualizer_Nature::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(NatureShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Visualizer_Nature::imgui()
{
    PROFILE_FUNCTION();

    const char * terrainTypes[] = { "Grassy Hills", "Rocky Cliffs", "Desert Sand", "Alpine" };
    ImGui::Combo("Terrain Type", &m_terrainType, terrainTypes, IM_ARRAYSIZE(terrainTypes));
    if (m_terrainType == 0)
    {
        ImGui::SliderFloat("Pond Level", &m_waterLevel, 0.05f, 0.60f);
    }
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }

}

void Visualizer_Nature::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (m_hasFrame)
    {
        m_sprite.setPosition(projector().getTransformedPosition());
        const float scale = projector().getTransformedScale();
        m_sprite.setScale({ scale, scale });

        if (m_shaderLoaded)
        {
            const sf::Vector2u textureSize = m_texture.getSize();
            m_shader.setUniform(
                "texelSize",
                sf::Glsl::Vec2(1.0f / textureSize.x, 1.0f / textureSize.y));
            m_shader.setUniform("terrainType", m_terrainType);
            m_shader.setUniform("waterLevel", m_waterLevel);
            window.draw(m_sprite, &m_shader);
        }
        else
        {
            window.draw(m_sprite);
        }
    }
}

void Visualizer_Nature::processEvent(
    const sf::Event & event,
    const sf::Vector2f & mouse)
{
    projector().processEvent(event, mouse);
}

void Visualizer_Nature::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_Nature");
    settings["m_terrainType"] = m_terrainType;
    settings["m_waterLevel"] = m_waterLevel;
}

void Visualizer_Nature::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_Nature");
    Settings::read(settings, "m_terrainType", m_terrainType);
    Settings::read(settings, "m_waterLevel", m_waterLevel);
}

void Visualizer_Nature::process(const TerrainFrame & data)
{
    PROFILE_FUNCTION();

    if (data.heightMap.empty() || data.heightMap.type() != CV_32F)
    {
        m_hasFrame = false;
        return;
    }

    m_topography = data.heightMap;
    m_topographySize = data.heightMap.size();

    m_hasFrame = projector().updateTexture(
        m_topography,
        m_projectedTopography,
        m_image,
        m_texture,
        m_sprite,
        true,
        "Failed to load the nature terrain texture.\n");
}
