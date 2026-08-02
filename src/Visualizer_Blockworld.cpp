#include "Visualizer_Blockworld.h"
#include "Profiler.hpp"

#include "imgui.h"

#include <algorithm>

namespace
{
    constexpr const char * BlockworldShaderPath = "shaders/shader_blockworld.frag";
}

void Visualizer_Blockworld::init()
{
    reloadShader();
}

void Visualizer_Blockworld::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(BlockworldShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Visualizer_Blockworld::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderInt("Block Size", &m_blockSize, 2, 48);
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }

}

void Visualizer_Blockworld::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (m_shaderLoaded)
    {
        const sf::Texture * terrainTexture = projector().terrainTexture();
        if (!terrainTexture) { return; }
        const sf::Vector2u textureSize = terrainTexture->getSize();
        m_shader.setUniform("texelSize", sf::Glsl::Vec2(
            1.0f / textureSize.x,
            1.0f / textureSize.y));
        m_shader.setUniform("blockSize", (float)m_blockSize);
        projector().drawTerrain(window, &m_shader);
    }
    else
    {
        projector().drawTerrain(window);
    }
}

void Visualizer_Blockworld::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    projector().processEvent(event, mouse);
}

void Visualizer_Blockworld::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_Blockworld");
    settings["m_blockSize"] = m_blockSize;
}

void Visualizer_Blockworld::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_Blockworld");
    Settings::read(settings, "m_blockSize", m_blockSize);
    m_blockSize = std::clamp(m_blockSize, 2, 48);
}
