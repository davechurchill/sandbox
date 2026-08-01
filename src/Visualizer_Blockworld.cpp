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

    if (!m_hasFrame)
    {
        return;
    }

    m_sprite.setPosition(projector().getTransformedPosition());
    const float scale = projector().getTransformedScale();
    m_sprite.setScale({ scale, scale });

    if (m_shaderLoaded)
    {
        const sf::Vector2u textureSize = m_texture.getSize();
        m_shader.setUniform("texelSize", sf::Glsl::Vec2(
            1.0f / textureSize.x,
            1.0f / textureSize.y));
        m_shader.setUniform("blockSize", (float)m_blockSize);
        window.draw(m_sprite, &m_shader);
    }
    else
    {
        window.draw(m_sprite);
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

void Visualizer_Blockworld::process(const TerrainFrame & data)
{
    PROFILE_FUNCTION();

    m_hasFrame = projector().updateTexture(
        data.heightMap,
        m_projectedTopography,
        m_image,
        m_texture,
        m_sprite,
        false,
        "Failed to load the Blockworld terrain texture.\n");
}
