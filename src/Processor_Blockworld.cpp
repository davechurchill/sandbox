#include "Processor_Blockworld.h"
#include "Profiler.hpp"

#include "imgui.h"

#include <algorithm>

namespace
{
    constexpr const char * BlockworldShaderPath = "shaders/shader_blockworld.frag";
}

void Processor_Blockworld::init()
{
    reloadShader();
}

void Processor_Blockworld::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(BlockworldShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Processor_Blockworld::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderInt("Block Size", &m_blockSize, 2, 48);
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }

}

void Processor_Blockworld::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (!m_hasFrame)
    {
        return;
    }

    if (m_shaderLoaded)
    {
        const sf::Vector2u textureSize = m_surface.texture().getSize();
        m_shader.setUniform("texelSize", sf::Glsl::Vec2(
            1.0f / textureSize.x,
            1.0f / textureSize.y));
        m_shader.setUniform("blockSize", (float)m_blockSize);
        m_surface.draw(window, projector(), &m_shader);
    }
    else
    {
        m_surface.draw(window, projector());
    }
}

void Processor_Blockworld::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    projector().processEvent(event, mouse);
}

void Processor_Blockworld::save(Save & save) const
{
    Save::Json & settings = save.section("Processor_Blockworld");
    settings["m_blockSize"] = m_blockSize;
}

void Processor_Blockworld::load(const Save & save)
{
    const Save::Json & currentSettings = save.section("Processor_Blockworld");
    const Save::Json & settings = currentSettings.empty()
        ? save.section("Processor_Minecraft")
        : currentSettings;
    Save::read(settings, "m_blockSize", m_blockSize);
    m_blockSize = std::clamp(m_blockSize, 2, 48);
}

void Processor_Blockworld::processTopography(const IntermediateData & data)
{
    PROFILE_FUNCTION();

    m_hasFrame = m_surface.update(
        data.topography,
        projector(),
        false,
        "Failed to load the Blockworld terrain texture.\n");
}
