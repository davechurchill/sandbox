#include "Visualizer_PrismaticGlacier.h"
#include "Profiler.hpp"

#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr const char * PrismaticGlacierShaderPath = "shaders/shader_prismatic_glacier.frag";
    constexpr float DefaultFacetSize = 18.0f;
    constexpr float DefaultIceDepthAbsorption = 1.15f;
    constexpr float DefaultIridescence = 0.80f;
    constexpr float DefaultFractureIntensity = 0.85f;
    constexpr float DefaultCausticSpeed = 0.65f;
}

void Visualizer_PrismaticGlacier::init()
{
    reloadShader();
}

void Visualizer_PrismaticGlacier::resetDefaults()
{
    m_facetSize = DefaultFacetSize;
    m_iceDepthAbsorption = DefaultIceDepthAbsorption;
    m_iridescence = DefaultIridescence;
    m_fractureIntensity = DefaultFractureIntensity;
    m_causticSpeed = DefaultCausticSpeed;
    m_time = 0.0f;
}

void Visualizer_PrismaticGlacier::clampSettings()
{
    m_facetSize = std::isfinite(m_facetSize) ? std::clamp(m_facetSize, 3.0f, 64.0f) : DefaultFacetSize;
    m_iceDepthAbsorption = std::isfinite(m_iceDepthAbsorption) ? std::clamp(m_iceDepthAbsorption, 0.10f, 3.0f) : DefaultIceDepthAbsorption;
    m_iridescence = std::isfinite(m_iridescence) ? std::clamp(m_iridescence, 0.0f, 2.0f) : DefaultIridescence;
    m_fractureIntensity = std::isfinite(m_fractureIntensity) ? std::clamp(m_fractureIntensity, 0.0f, 2.0f) : DefaultFractureIntensity;
    m_causticSpeed = std::isfinite(m_causticSpeed) ? std::clamp(m_causticSpeed, 0.0f, 2.0f) : DefaultCausticSpeed;
}

void Visualizer_PrismaticGlacier::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(PrismaticGlacierShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Visualizer_PrismaticGlacier::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderFloat("Facet Size", &m_facetSize, 3.0f, 64.0f, "%.0f px");
    ImGui::SliderFloat("Ice Depth / Absorption", &m_iceDepthAbsorption, 0.10f, 3.0f, "%.2f");
    ImGui::SliderFloat("Iridescence", &m_iridescence, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Fracture Intensity", &m_fractureIntensity, 0.0f, 2.0f, "%.2f");
    ImGui::SliderFloat("Caustic Speed", &m_causticSpeed, 0.0f, 2.0f, "%.2f");

    if (ImGui::Button("Reset Defaults"))
    {
        resetDefaults();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }
}

void Visualizer_PrismaticGlacier::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (m_shaderLoaded)
    {
        const sf::Texture * terrainTexture = projector().terrainTexture(true);
        if (!terrainTexture) { return; }
        const sf::Vector2u textureSize = terrainTexture->getSize();
        m_shader.setUniform("texelSize", sf::Glsl::Vec2(1.0f / textureSize.x, 1.0f / textureSize.y));
        m_shader.setUniform("facetSize", m_facetSize);
        m_shader.setUniform("iceDepthAbsorption", m_iceDepthAbsorption);
        m_shader.setUniform("iridescence", m_iridescence);
        m_shader.setUniform("fractureIntensity", m_fractureIntensity);
        m_shader.setUniform("causticSpeed", m_causticSpeed);
        m_shader.setUniform("u_time", m_time);
        projector().drawTerrain(window, &m_shader, true);
    }
    else
    {
        projector().drawTerrain(window, nullptr, true);
    }
}

void Visualizer_PrismaticGlacier::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    projector().processEvent(event, mouse);
}

void Visualizer_PrismaticGlacier::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_PrismaticGlacier");
    settings["m_facetSize"] = m_facetSize;
    settings["m_iceDepthAbsorption"] = m_iceDepthAbsorption;
    settings["m_iridescence"] = m_iridescence;
    settings["m_fractureIntensity"] = m_fractureIntensity;
    settings["m_causticSpeed"] = m_causticSpeed;
}

void Visualizer_PrismaticGlacier::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_PrismaticGlacier");
    Settings::read(settings, "m_facetSize", m_facetSize);
    Settings::read(settings, "m_iceDepthAbsorption", m_iceDepthAbsorption);
    Settings::read(settings, "m_iridescence", m_iridescence);
    Settings::read(settings, "m_fractureIntensity", m_fractureIntensity);
    Settings::read(settings, "m_causticSpeed", m_causticSpeed);
    clampSettings();
}

void Visualizer_PrismaticGlacier::process(const TerrainFrame & data)
{
    PROFILE_FUNCTION();

    const float deltaTime = std::isfinite(data.deltaTime) ? std::clamp(data.deltaTime, 0.0f, 0.10f) : 0.0f;
    m_time = std::fmod(m_time + deltaTime, 4096.0f);
}
