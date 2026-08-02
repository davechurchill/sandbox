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
    ImGui::Checkbox("Texture Detail", &m_textureDetail);
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

    if (m_shaderLoaded)
    {
        const sf::Texture * terrainTexture = projector().terrainTexture(true);
        if (!terrainTexture) { return; }
        const sf::Vector2u textureSize = terrainTexture->getSize();
        m_shader.setUniform(
            "texelSize",
            sf::Glsl::Vec2(1.0f / textureSize.x, 1.0f / textureSize.y));
        m_shader.setUniform("terrainType", m_terrainType);
        m_shader.setUniform("waterLevel", m_waterLevel);
        m_shader.setUniform("textureStrength", m_textureDetail ? 1.0f : 0.0f);
        projector().drawTerrain(window, &m_shader, true);
    }
    else
    {
        projector().drawTerrain(window, nullptr, true);
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
    settings["m_textureDetail"] = m_textureDetail;
}

void Visualizer_Nature::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_Nature");
    Settings::read(settings, "m_terrainType", m_terrainType);
    Settings::read(settings, "m_waterLevel", m_waterLevel);
    Settings::read(settings, "m_textureDetail", m_textureDetail);
}
