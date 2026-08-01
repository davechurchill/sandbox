#include "Overlay_BFS.h"

#include "Profiler.hpp"
#include "VectorField.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
    constexpr size_t MaximumActiveParticles = 250000;

    double wrapCoordinate(double value, int size)
    {
        const double extent = (double)std::max(size - 1, 1);
        value = std::fmod(value, extent);
        return value < 0.0 ? value + extent : value;
    }
}

void Overlay_BFS::resetParticles()
{
    m_particles.clear();
    m_spawnAccumulator = 0.0;
}

void Overlay_BFS::updateParticles(const cv::Mat & terrain, float deltaTime)
{
    if (terrain.empty() || terrain.type() != CV_32F)
    {
        return;
    }

    const cv::Size size = terrain.size();
    if (m_resetRequested || size != m_topographySize)
    {
        m_topographySize = size;
        resetParticles();
        m_resetRequested = false;
    }

    const float dt = std::clamp(deltaTime, 0.0f, 0.05f);
    m_spawnAccumulator += std::max(m_spawnRate, 0.0f) * dt;
    int particlesToSpawn = (int)std::floor(m_spawnAccumulator);
    m_spawnAccumulator -= particlesToSpawn;
    const size_t availableSlots = MaximumActiveParticles > m_particles.size()
        ? MaximumActiveParticles - m_particles.size()
        : 0;
    particlesToSpawn = std::min(particlesToSpawn, (int)availableSlots);
    if (availableSlots == 0)
    {
        m_spawnAccumulator = 0.0;
    }

    std::uniform_real_distribution<double> yDistribution(0.0, size.height - 0.001);
    for (int i = 0; i < particlesToSpawn; i++)
    {
        Particle particle;
        particle.position = { 0.0, yDistribution(m_random) };
        m_particles.push_back(std::move(particle));
    }

    const int cellSize = std::max(m_cellSize, 1);
    const int trailLength = std::max(m_trailLength, 1);
    const cv::Mat directions = VectorField::computeBFS(
        terrain,
        cellSize,
        std::max(m_heightPenalty, 0.0f));
    if (directions.empty())
    {
        return;
    }

    const double movement = std::max(m_particleSpeed, 0.0f)
        * dt;

    for (Particle & particle : m_particles)
    {
        particle.position.x = std::clamp(
            particle.position.x,
            0.0,
            (double)(size.width - 1));
        particle.position.y = std::clamp(
            particle.position.y,
            0.0,
            (double)(size.height - 1));

        const int fieldX = std::clamp(
            (int)(particle.position.x / cellSize),
            0,
            directions.cols - 1);
        const int fieldY = std::clamp(
            (int)(particle.position.y / cellSize),
            0,
            directions.rows - 1);
        cv::Vec2d direction = directions.at<cv::Vec2d>(fieldY, fieldX);
        if (!std::isfinite(direction[0]) || !std::isfinite(direction[1])
            || std::abs(direction[0]) + std::abs(direction[1]) < 0.000001)
        {
            direction = { 1.0, 0.0 };
        }

        particle.position.x += direction[0] * movement;
        particle.position.y += direction[1] * movement;

        const bool yOutOfBounds = particle.position.y < 0.0
            || particle.position.y >= size.height - 1;
        if (particle.position.x < 0.0)
        {
            particle.position.x = 0.0;
            particle.trail.clear();
        }
        if (yOutOfBounds)
        {
            particle.position.y = wrapCoordinate(particle.position.y, size.height);
            particle.trail.clear();
        }

        if (particle.position.x >= size.width - 1)
        {
            continue;
        }

        particle.trail.push_back(particle.position);
        while ((int)particle.trail.size() > trailLength)
        {
            particle.trail.erase(particle.trail.begin());
        }
    }

    m_particles.erase(
        std::remove_if(
            m_particles.begin(),
            m_particles.end(),
            [&](const Particle & particle)
            {
                return particle.position.x >= size.width - 1;
            }),
        m_particles.end());
}

void Overlay_BFS::updateTexture(
    const cv::Mat & terrain,
    TopographyProcessor & processor)
{
    cv::Mat particleGrid = cv::Mat::zeros(terrain.rows, terrain.cols, CV_8U);
    const int trailLength = std::max(m_trailLength, 1);
    const auto setParticlePixel = [&](const sf::Vector2<double> & position, uint8_t value)
    {
        const int x = std::clamp((int)std::round(position.x), 0, particleGrid.cols - 1);
        const int y = std::clamp((int)std::round(position.y), 0, particleGrid.rows - 1);
        uint8_t & pixel = particleGrid.at<uint8_t>(y, x);
        pixel = std::max(pixel, value);
    };

    for (const Particle & particle : m_particles)
    {
        for (int i = 0; i < (int)particle.trail.size(); i++)
        {
            const int intensity = 255
                - (trailLength - i - 1) * 255 / (trailLength + 1);
            setParticlePixel(
                particle.trail[i],
                (uint8_t)std::clamp(intensity, 0, 255));
        }
        setParticlePixel(particle.position, 255);
    }

    cv::Mat transformedParticles;
    processor.projector().project(particleGrid, transformedParticles);
    if (transformedParticles.empty())
    {
        return;
    }

    cv::Mat rgba(
        transformedParticles.rows,
        transformedParticles.cols,
        CV_8UC4,
        cv::Scalar(0, 0, 0, 0));
    for (int y = 0; y < transformedParticles.rows; y++)
    {
        for (int x = 0; x < transformedParticles.cols; x++)
        {
            const uint8_t intensity = transformedParticles.at<uint8_t>(y, x);
            if (intensity > 0)
            {
                rgba.at<cv::Vec4b>(y, x) = { 0, intensity, 255, 255 };
            }
        }
    }

    sf::Image image;
    image.resize({ (unsigned int)rgba.cols, (unsigned int)rgba.rows }, rgba.ptr());
    if (!m_texture.loadFromImage(image))
    {
        std::cerr << "Failed to load the BFS particle texture.\n";
        return;
    }
    m_sprite.setTexture(m_texture, true);
}

void Overlay_BFS::initOverlay()
{
    m_resetRequested = true;
    if (!m_shader.loadFromFile("shaders/shader_vector_fields.frag", sf::Shader::Type::Fragment))
    {
        std::cerr << "Failed to load the BFS shader.\n";
    }
    else
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Overlay_BFS::imguiOverlay()
{
    PROFILE_FUNCTION();

    ImGui::TextUnformatted("Mode: BFS");
    ImGui::SliderFloat("Spawn Rate", &m_spawnRate, 0.0f, 20000.0f, "%.0f / sec");
    ImGui::Text("Active Particles: %zu", m_particles.size());
    ImGui::SliderInt("Trail Length", &m_trailLength, 1, 32);
    ImGui::SliderFloat("Particle Speed", &m_particleSpeed, 0.0f, 1000.0f, "%.1f");
    ImGui::SliderFloat("Particle Alpha", &m_particleAlpha, 0.0f, 1.0f);
    ImGui::SliderInt("Cell Size", &m_cellSize, 1, 128);
    ImGui::SliderFloat("Height Penalty", &m_heightPenalty, 0.0f, 5.0f, "%.2f");

    if (ImGui::Button("Reset Particles"))
    {
        m_resetRequested = true;
    }
    if (ImGui::Button("Reload Shader"))
    {
        if (!m_shader.loadFromFile("shaders/shader_vector_fields.frag", sf::Shader::Type::Fragment))
        {
            std::cerr << "Failed to reload the BFS shader.\n";
        }
        else
        {
            m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
        }
    }
}

void Overlay_BFS::processTopographyOverlay(
    const IntermediateData & data,
    TopographyProcessor & processor)
{
    PROFILE_FUNCTION();
    updateParticles(data.topography, data.deltaTime);
    if (!data.topography.empty())
    {
        updateTexture(data.topography, processor);
    }
}

void Overlay_BFS::renderOverlay(
    sf::RenderWindow & window,
    TopographyProcessor & processor)
{
    PROFILE_FUNCTION();
    if (m_texture.getSize().x == 0 || m_texture.getSize().y == 0)
    {
        return;
    }

    SandBoxProjector & projector = processor.projector();
    m_sprite.setPosition(projector.getTransformedPosition());
    const float scale = projector.getTransformedScale();
    m_sprite.setScale({ scale, scale });
    m_shader.setUniform("particleAlpha", m_particleAlpha);
    m_shader.setUniform("overlayOnly", true);
    m_shader.setUniform("reverseDepthAlpha", false);
    window.draw(m_sprite, &m_shader);
}

void Overlay_BFS::processOverlayEvent(
    const sf::Event &,
    const sf::Vector2f &,
    TopographyProcessor &)
{
}

void Overlay_BFS::saveOverlay(Save &) const
{
}

void Overlay_BFS::loadOverlay(const Save &)
{
}
