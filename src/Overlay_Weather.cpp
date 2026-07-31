#include "Overlay_Weather.h"

#include "Profiler.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;

    bool isTerrainCell(float height)
    {
        return std::isfinite(height) && height > 0.001f && height < 0.999f;
    }
}

void Overlay_Weather::resetParticles()
{
    m_particles.clear();
}

float Overlay_Weather::sampleHeight(
    const cv::Mat & terrain,
    const cv::Point2f & position) const
{
    if (terrain.empty() || terrain.type() != CV_32F
        || position.x < 0.0f || position.y < 0.0f
        || position.x >= terrain.cols || position.y >= terrain.rows)
    {
        return std::numeric_limits<float>::quiet_NaN();
    }

    const int x = std::clamp((int)std::round(position.x), 0, terrain.cols - 1);
    const int y = std::clamp((int)std::round(position.y), 0, terrain.rows - 1);
    const float height = terrain.at<float>(y, x);
    return isTerrainCell(height)
        ? height
        : std::numeric_limits<float>::quiet_NaN();
}

void Overlay_Weather::initializeParticle(
    Particle & particle,
    const cv::Mat & terrain,
    bool randomAltitude)
{
    if (terrain.empty())
    {
        particle = Particle{};
        return;
    }

    std::uniform_real_distribution<float> xDistribution(0.0f, std::max(0.0f, terrain.cols - 0.001f));
    std::uniform_real_distribution<float> yDistribution(0.0f, std::max(0.0f, terrain.rows - 0.001f));
    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);
    std::uniform_real_distribution<float> phaseDistribution(0.0f, Pi * 2.0f);
    std::uniform_real_distribution<float> sizeDistribution(0.72f, 1.28f);
    std::uniform_real_distribution<float> opacityDistribution(0.65f, 1.0f);

    float terrainHeight = std::numeric_limits<float>::quiet_NaN();
    for (int attempt = 0; attempt < 64; attempt++)
    {
        particle.position = { xDistribution(m_random), yDistribution(m_random) };
        terrainHeight = sampleHeight(terrain, particle.position);
        if (std::isfinite(terrainHeight))
        {
            break;
        }
    }

    if (!std::isfinite(terrainHeight))
    {
        particle.position = { terrain.cols * 0.5f, terrain.rows * 0.5f };
        terrainHeight = sampleHeight(terrain, particle.position);
        if (!std::isfinite(terrainHeight))
        {
            terrainHeight = 0.0f;
        }
    }

    const Mode mode = (Mode)m_mode;
    if (mode == Mode::Rain || mode == Mode::Snow)
    {
        const float minimumAltitude = std::clamp(terrainHeight + 0.03f, 0.20f, 0.98f);
        particle.altitude = randomAltitude
            ? minimumAltitude + (1.0f - minimumAltitude) * unitDistribution(m_random)
            : 1.0f;
    }
    else if (mode == Mode::Fog)
    {
        particle.altitude = 0.25f + unitDistribution(m_random) * 0.32f;
    }
    else
    {
        particle.altitude = 0.72f + unitDistribution(m_random) * 0.25f;
    }
    particle.phase = phaseDistribution(m_random);
    particle.size = sizeDistribution(m_random);
    particle.opacity = opacityDistribution(m_random);
}

void Overlay_Weather::wrapPosition(cv::Point2f & position, const cv::Size & size) const
{
    if (size.width <= 0 || size.height <= 0)
    {
        return;
    }

    position.x = std::fmod(position.x, (float)size.width);
    position.y = std::fmod(position.y, (float)size.height);
    if (position.x < 0.0f) { position.x += size.width; }
    if (position.y < 0.0f) { position.y += size.height; }
}

int Overlay_Weather::targetParticleCount() const
{
    int maximum = 0;
    switch ((Mode)m_mode)
    {
    case Mode::Rain: maximum = 700; break;
    case Mode::Snow: maximum = 280; break;
    case Mode::Fog: maximum = 75; break;
    case Mode::Clouds: maximum = 42; break;
    }
    return (int)std::round(maximum * std::clamp(m_intensity, 0.0f, 1.0f));
}

void Overlay_Weather::updateParticles(const cv::Mat & terrain, float deltaTime)
{
    const int target = targetParticleCount();
    while ((int)m_particles.size() < target)
    {
        Particle particle;
        initializeParticle(particle, terrain, true);
        m_particles.push_back(particle);
    }
    if ((int)m_particles.size() > target)
    {
        m_particles.resize(target);
    }

    const float dt = std::clamp(deltaTime, 0.0f, 0.05f);
    const float terrainScale = (float)std::min(terrain.cols, terrain.rows);
    const Mode mode = (Mode)m_mode;
    const bool precipitation = mode == Mode::Rain || mode == Mode::Snow;
    const float windScale = precipitation
        ? terrainScale * 0.10f
        : terrainScale * (mode == Mode::Clouds ? 0.055f : 0.025f);

    for (Particle & particle : m_particles)
    {
        particle.phase = std::fmod(particle.phase + dt * (mode == Mode::Snow ? 2.1f : 0.45f), Pi * 2.0f);
        particle.position.x += m_windX * windScale * dt;
        particle.position.y += m_windY * windScale * dt;
        if (mode == Mode::Snow)
        {
            particle.position.x += std::sin(particle.phase) * terrainScale * 0.012f * dt;
        }
        wrapPosition(particle.position, terrain.size());

        const float terrainHeight = sampleHeight(terrain, particle.position);
        if (!std::isfinite(terrainHeight))
        {
            initializeParticle(particle, terrain, true);
            continue;
        }

        if (mode == Mode::Rain)
        {
            particle.altitude -= 0.65f * m_fallSpeed * dt;
        }
        else if (mode == Mode::Snow)
        {
            particle.altitude -= 0.13f * m_fallSpeed * dt;
        }

        if (precipitation && particle.altitude <= terrainHeight + 0.01f)
        {
            initializeParticle(particle, terrain, false);
        }
    }
}

void Overlay_Weather::renderPrecipitation(sf::RenderWindow & window, bool snow)
{
    if (!m_processor || m_particles.empty())
    {
        return;
    }

    SandBoxProjector & projector = m_processor->projector();
    const cv::Mat projection = projector.getProjectionMatrix();
    const float scale = projector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    std::vector<cv::Point2f> points;
    points.reserve(m_particles.size());
    for (const Particle & particle : m_particles)
    {
        points.push_back(particle.position);
    }
    cv::perspectiveTransform(points, points, projection);

    sf::VertexArray vertices(sf::Lines);
    const sf::Vector2f origin = projector.getTransformedPosition();
    for (size_t i = 0; i < m_particles.size(); i++)
    {
        if (!std::isfinite(points[i].x) || !std::isfinite(points[i].y))
        {
            continue;
        }

        const float height = sampleHeight(m_topography, m_particles[i].position);
        if (!std::isfinite(height))
        {
            continue;
        }
        const float clearance = std::max(m_particles[i].altitude - height, 0.0f);
        const sf::Vector2f position(
            origin.x + points[i].x * scale,
            origin.y + points[i].y * scale - clearance * (snow ? 18.0f : 28.0f));
        const sf::Uint8 alpha = (sf::Uint8)std::clamp(
            (int)(95 + 135 * m_particles[i].opacity * m_intensity),
            0,
            255);

        if (snow)
        {
            const float radius = (1.5f + 2.1f * m_particles[i].size) * m_elementSize;
            const sf::Color color(245, 250, 255, alpha);
            vertices.append(sf::Vertex({ position.x - radius, position.y }, color));
            vertices.append(sf::Vertex({ position.x + radius, position.y }, color));
            vertices.append(sf::Vertex({ position.x, position.y - radius }, color));
            vertices.append(sf::Vertex({ position.x, position.y + radius }, color));
        }
        else
        {
            const float length = (7.0f + 8.0f * m_particles[i].size) * m_elementSize;
            const sf::Vector2f tail(
                position.x - m_windX * length * 0.45f,
                position.y - length);
            const sf::Color startColor(135, 190, 255, alpha);
            const sf::Color tailColor(185, 220, 255, (sf::Uint8)(alpha * 0.25f));
            vertices.append(sf::Vertex(position, startColor));
            vertices.append(sf::Vertex(tail, tailColor));
        }
    }
    window.draw(vertices, sf::BlendAlpha);
}

void Overlay_Weather::renderAtmosphere(sf::RenderWindow & window, bool clouds)
{
    if (!m_processor || m_particles.empty())
    {
        return;
    }

    SandBoxProjector & projector = m_processor->projector();
    const cv::Mat projection = projector.getProjectionMatrix();
    const float scale = projector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    std::vector<cv::Point2f> points;
    points.reserve(m_particles.size());
    for (const Particle & particle : m_particles)
    {
        points.push_back(particle.position);
    }
    cv::perspectiveTransform(points, points, projection);

    const sf::Vector2f origin = projector.getTransformedPosition();
    for (size_t i = 0; i < m_particles.size(); i++)
    {
        if (!std::isfinite(points[i].x) || !std::isfinite(points[i].y))
        {
            continue;
        }

        const Particle & particle = m_particles[i];
        const float height = sampleHeight(m_topography, particle.position);
        if (!std::isfinite(height))
        {
            continue;
        }

        const float terrainFactor = clouds
            ? std::clamp((particle.altitude - height) / 0.28f, 0.0f, 1.0f)
            : std::clamp((0.68f - height) / 0.50f, 0.08f, 1.0f);
        const float pulse = 0.90f + std::sin(particle.phase) * 0.10f;
        const float alphaScale = particle.opacity * m_intensity * terrainFactor * pulse;
        if (alphaScale <= 0.002f)
        {
            continue;
        }

        const sf::Vector2f position(
            origin.x + points[i].x * scale,
            origin.y + points[i].y * scale);
        const float radius = m_elementSize * particle.size * (clouds ? 42.0f : 34.0f);

        if (clouds)
        {
            sf::CircleShape shadow(radius * 0.92f, 32);
            shadow.setOrigin(shadow.getRadius(), shadow.getRadius());
            shadow.setPosition(position.x + radius * 0.24f, position.y + radius * 0.28f);
            shadow.setScale(1.35f, 0.70f);
            shadow.setFillColor(sf::Color(25, 32, 42, (sf::Uint8)(38 * alphaScale)));
            window.draw(shadow);

            const sf::Uint8 alpha = (sf::Uint8)(105 * alphaScale);
            sf::CircleShape cloud(radius, 36);
            cloud.setOrigin(radius, radius);
            cloud.setPosition(position);
            cloud.setScale(1.35f, 0.72f);
            cloud.setFillColor(sf::Color(235, 240, 244, alpha));
            window.draw(cloud);

            sf::CircleShape lobe(radius * 0.66f, 30);
            lobe.setOrigin(lobe.getRadius(), lobe.getRadius());
            lobe.setFillColor(sf::Color(248, 250, 252, alpha));
            lobe.setPosition(position.x - radius * 0.55f, position.y - radius * 0.12f);
            window.draw(lobe);
            lobe.setPosition(position.x + radius * 0.50f, position.y - radius * 0.08f);
            lobe.setScale(0.82f, 0.82f);
            window.draw(lobe);
        }
        else
        {
            sf::CircleShape fog(radius, 32);
            fog.setOrigin(radius, radius);
            fog.setPosition(position);
            fog.setScale(1.65f, 0.72f);
            fog.setFillColor(sf::Color(205, 220, 225, (sf::Uint8)(48 * alphaScale)));
            window.draw(fog);
        }
    }
}

void Overlay_Weather::initOverlay()
{
    resetParticles();
}

void Overlay_Weather::imguiOverlay()
{
    PROFILE_FUNCTION();

    const char * modes[] = { "Rain", "Snow", "Fog", "Clouds" };
    if (ImGui::Combo("Weather Type", &m_mode, modes, IM_ARRAYSIZE(modes)))
    {
        resetParticles();
    }
    ImGui::SliderFloat("Intensity", &m_intensity, 0.0f, 1.0f);
    ImGui::SliderFloat("Wind X", &m_windX, -1.0f, 1.0f);
    ImGui::SliderFloat("Wind Y", &m_windY, -1.0f, 1.0f);
    ImGui::SliderFloat("Element Size", &m_elementSize, 0.5f, 2.0f);
    if ((Mode)m_mode == Mode::Rain || (Mode)m_mode == Mode::Snow)
    {
        ImGui::SliderFloat("Fall Speed", &m_fallSpeed, 0.2f, 3.0f, "%.1fx");
    }
    if (ImGui::Button("Reset Weather"))
    {
        resetParticles();
    }
}

void Overlay_Weather::processTopographyOverlay(
    const IntermediateData & data,
    TopographyProcessor & processor)
{
    if (data.topography.empty() || data.topography.type() != CV_32F)
    {
        return;
    }

    m_processor = &processor;
    if (m_topographySize.width > 0 && m_topographySize.height > 0
        && m_topographySize != data.topography.size())
    {
        const float xScale = (float)data.topography.cols / m_topographySize.width;
        const float yScale = (float)data.topography.rows / m_topographySize.height;
        for (Particle & particle : m_particles)
        {
            particle.position.x *= xScale;
            particle.position.y *= yScale;
        }
    }

    m_topography = data.topography;
    m_topographySize = data.topography.size();
    updateParticles(m_topography, data.deltaTime);
}

void Overlay_Weather::renderOverlay(
    sf::RenderWindow & window,
    TopographyProcessor & processor)
{
    m_processor = &processor;
    switch ((Mode)m_mode)
    {
    case Mode::Rain: renderPrecipitation(window, false); break;
    case Mode::Snow: renderPrecipitation(window, true); break;
    case Mode::Fog: renderAtmosphere(window, false); break;
    case Mode::Clouds: renderAtmosphere(window, true); break;
    }
}

void Overlay_Weather::processOverlayEvent(
    const sf::Event &,
    const sf::Vector2f &,
    TopographyProcessor &)
{
}

void Overlay_Weather::saveOverlay(Save &) const
{
}

void Overlay_Weather::loadOverlay(const Save &)
{
}
