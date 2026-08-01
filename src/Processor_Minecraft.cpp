#include "Processor_Minecraft.h"
#include "Profiler.hpp"
#include "Tools.h"

#include "imgui.h"

#include <algorithm>
#include <iostream>

namespace
{
    constexpr const char * MinecraftShaderPath = "shaders/shader_minecraft.frag";
}

void Processor_Minecraft::init()
{
    reloadShader();
}

void Processor_Minecraft::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(MinecraftShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Processor_Minecraft::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderInt("Block Size", &m_blockSize, 2, 48);
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }

}

void Processor_Minecraft::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (!m_hasFrame)
    {
        return;
    }

    m_sprite.setPosition(m_projector.getTransformedPosition());
    const float scale = m_projector.getTransformedScale();
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

void Processor_Minecraft::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    m_projector.processEvent(event, mouse);
}

void Processor_Minecraft::save(Save & save) const
{
    Save::Json & settings = save.section("Processor_Minecraft");
    settings["m_blockSize"] = m_blockSize;
    m_projector.save(save);
}

void Processor_Minecraft::load(const Save & save)
{
    const Save::Json & settings = save.section("Processor_Minecraft");
    Save::read(settings, "m_blockSize", m_blockSize);
    m_blockSize = std::clamp(m_blockSize, 2, 48);
    m_projector.load(save);
}

void Processor_Minecraft::processTopography(const IntermediateData & data)
{
    PROFILE_FUNCTION();

    m_projector.project(data.topography, m_projectedTopography);
    if (m_projectedTopography.empty())
    {
        m_hasFrame = false;
        return;
    }

    m_image = Tools::matToSfImage(m_projectedTopography);
    if (!m_texture.loadFromImage(m_image))
    {
        std::cerr << "Failed to load the Minecraft terrain texture.\n";
        return;
    }
    m_texture.setSmooth(false);
    m_sprite.setTexture(m_texture, true);
    m_hasFrame = true;
}
