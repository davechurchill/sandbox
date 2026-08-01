#include "Processor_Nature.h"
#include "Profiler.hpp"
#include "Tools.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
    constexpr const char * NatureShaderPath = "shaders/shader_nature.frag";

    bool isTerrainCell(float height)
    {
        return std::isfinite(height) && height > 0.001f && height < 0.999f;
    }
}

void Processor_Nature::init()
{
    reloadShader();
}

void Processor_Nature::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(NatureShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Processor_Nature::imgui()
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

    ImGui::Separator();
    m_projector.imgui();
}

bool Processor_Nature::isTerrainWalkable(
    const cv::Mat & terrain,
    const cv::Point2f & position) const
{
    if (terrain.empty() || terrain.type() != CV_32F
        || position.x < 0.0f || position.y < 0.0f
        || position.x >= terrain.cols || position.y >= terrain.rows)
    {
        return false;
    }

    const int x = std::clamp((int)std::round(position.x), 0, terrain.cols - 1);
    const int y = std::clamp((int)std::round(position.y), 0, terrain.rows - 1);
    const float height = terrain.at<float>(y, x);
    return isTerrainCell(height) && (m_terrainType != 0 || height > m_waterLevel);
}

void Processor_Nature::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (m_hasFrame)
    {
        m_sprite.setPosition(m_projector.getTransformedPosition());
        const float scale = m_projector.getTransformedScale();
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

void Processor_Nature::processEvent(
    const sf::Event & event,
    const sf::Vector2f & mouse)
{
    m_projector.processEvent(event, mouse);
}

void Processor_Nature::save(Save & save) const
{
    Save::Json & settings = save.section("Processor_Nature");
    settings["m_terrainType"] = m_terrainType;
    settings["m_waterLevel"] = m_waterLevel;
    m_projector.save(save);
}

void Processor_Nature::load(const Save & save)
{
    const Save::Json & settings = save.section("Processor_Nature");
    Save::read(settings, "m_terrainType", m_terrainType);
    Save::read(settings, "m_waterLevel", m_waterLevel);
    m_projector.load(save);
}

void Processor_Nature::processTopography(const IntermediateData & data)
{
    PROFILE_FUNCTION();

    if (data.topography.empty() || data.topography.type() != CV_32F)
    {
        m_hasFrame = false;
        return;
    }

    m_topography = data.topography;
    m_topographySize = data.topography.size();

    m_projector.project(m_topography, m_projectedTopography);
    if (m_projectedTopography.empty())
    {
        m_hasFrame = false;
        return;
    }

    m_image = Tools::matToSfImage(m_projectedTopography);
    if (!m_texture.loadFromImage(m_image))
    {
        std::cerr << "Failed to load the nature terrain texture.\n";
        return;
    }
    m_texture.setSmooth(true);
    m_sprite.setTexture(m_texture, true);
    m_hasFrame = true;
}
