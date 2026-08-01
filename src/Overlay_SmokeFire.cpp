#include "Overlay_SmokeFire.h"

#include "Profiler.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;

    bool isTerrainHeight(float height)
    {
        return std::isfinite(height) && height > 0.001f && height < 0.999f;
    }
}

float Overlay_SmokeFire::sampleHeight(const cv::Point2f & position) const
{
    if (m_topography.empty() || m_topography.type() != CV_32F
        || position.x < 0.0f || position.y < 0.0f
        || position.x >= m_topography.cols || position.y >= m_topography.rows)
    {
        return std::numeric_limits<float>::quiet_NaN();
    }

    const int x = std::clamp((int)std::round(position.x), 0, m_topography.cols - 1);
    const int y = std::clamp((int)std::round(position.y), 0, m_topography.rows - 1);
    const float height = m_topography.at<float>(y, x);
    return isTerrainHeight(height)
        ? height
        : std::numeric_limits<float>::quiet_NaN();
}

bool Overlay_SmokeFire::isBurnable(const cv::Point2f & position) const
{
    const float height = sampleHeight(position);
    if (!std::isfinite(height) || height < 0.045f || height > 0.97f)
    {
        return false;
    }
    return !m_processor || m_processor->isTerrainWalkable(m_topography, position);
}

bool Overlay_SmokeFire::mapMouseToTerrain(
    const sf::Vector2f & mouse,
    TopographyProcessor & processor,
    cv::Point2f & terrainPosition) const
{
    if (m_topography.empty())
    {
        return false;
    }

    SandBoxProjector & projector = processor.projector();
    const cv::Mat projection = projector.getProjectionMatrix();
    const float scale = projector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return false;
    }

    cv::Mat inverseProjection;
    if (cv::invert(projection, inverseProjection) == 0.0)
    {
        return false;
    }

    const sf::Vector2f local = (mouse - projector.getTransformedPosition()) / scale;
    std::vector<cv::Point2f> point = { { local.x, local.y } };
    cv::perspectiveTransform(point, point, inverseProjection);
    terrainPosition = point.front();
    return isBurnable(terrainPosition);
}

float Overlay_SmokeFire::fireIntensity(const Fire & fire) const
{
    const float fadeIn = std::clamp(fire.age / 0.35f, 0.0f, 1.0f);
    const float remaining = std::max(0.0f, fire.lifetime - fire.age);
    const float fadeOut = std::clamp(remaining / 1.6f, 0.0f, 1.0f);
    return fadeIn * fadeOut;
}

bool Overlay_SmokeFire::ignite(const cv::Point2f & position)
{
    if (!isBurnable(position))
    {
        return false;
    }

    const float mergeDistanceSquared = m_fireSize * m_fireSize * 0.36f;
    for (Fire & fire : m_fires)
    {
        const float dx = fire.position.x - position.x;
        const float dy = fire.position.y - position.y;
        if (dx * dx + dy * dy <= mergeDistanceSquared)
        {
            fire.age = std::min(fire.age, 0.25f);
            fire.lifetime = std::max(fire.lifetime, m_fireLifetime);
            return true;
        }
    }
    if (m_fires.size() >= MaximumFires)
    {
        return false;
    }

    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);
    Fire fire;
    fire.position = position;
    fire.lifetime = m_fireLifetime * (0.82f + unitDistribution(m_random) * 0.36f);
    fire.spreadTimer = 0.45f + unitDistribution(m_random) * 0.75f;
    m_fires.push_back(fire);
    return true;
}

void Overlay_SmokeFire::spawnFlame(const Fire & fire)
{
    if (m_flames.size() >= MaximumFlames)
    {
        return;
    }

    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);
    std::uniform_real_distribution<float> signedDistribution(-1.0f, 1.0f);
    const float angle = unitDistribution(m_random) * Pi * 2.0f;
    const float distance = std::sqrt(unitDistribution(m_random)) * m_fireSize * 0.38f;

    FlameParticle particle;
    particle.position = {
        fire.position.x + std::cos(angle) * distance,
        fire.position.y + std::sin(angle) * distance };
    const float terrainHeight = sampleHeight(particle.position);
    particle.altitude = (std::isfinite(terrainHeight) ? terrainHeight : sampleHeight(fire.position)) + 0.006f;
    particle.velocity = {
        signedDistribution(m_random) * 2.4f,
        signedDistribution(m_random) * 2.4f };
    particle.riseSpeed = 0.075f + unitDistribution(m_random) * 0.13f;
    particle.lifetime = 0.28f + unitDistribution(m_random) * 0.58f;
    particle.size = m_fireSize * (0.10f + unitDistribution(m_random) * 0.17f);
    particle.phase = unitDistribution(m_random) * Pi * 2.0f;

    const float heat = unitDistribution(m_random);
    const float colorVariation = unitDistribution(m_random);
    if (heat < 0.16f)
    {
        particle.color = sf::Color(255, 238, (std::uint8_t)(125 + colorVariation * 85.0f));
    }
    else if (heat < 0.44f)
    {
        particle.color = sf::Color(255, (std::uint8_t)(175 + colorVariation * 65.0f), (std::uint8_t)(18 + colorVariation * 45.0f));
    }
    else if (heat < 0.78f)
    {
        particle.color = sf::Color(255, (std::uint8_t)(82 + colorVariation * 88.0f), (std::uint8_t)(colorVariation * 20.0f));
    }
    else
    {
        particle.color = sf::Color((std::uint8_t)(215 + colorVariation * 40.0f), (std::uint8_t)(30 + colorVariation * 65.0f), 2);
    }
    m_flames.push_back(particle);
}

void Overlay_SmokeFire::spawnSmoke(const Fire & fire)
{
    if (m_smoke.size() >= MaximumSmoke)
    {
        return;
    }

    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);
    std::uniform_real_distribution<float> signedDistribution(-1.0f, 1.0f);
    const float angle = unitDistribution(m_random) * Pi * 2.0f;
    const float distance = std::sqrt(unitDistribution(m_random)) * m_fireSize * 0.32f;

    Smoke particle;
    particle.position = {
        fire.position.x + std::cos(angle) * distance,
        fire.position.y + std::sin(angle) * distance };
    const float terrainHeight = sampleHeight(particle.position);
    particle.altitude = (std::isfinite(terrainHeight) ? terrainHeight : sampleHeight(fire.position)) + 0.015f;
    particle.velocity = { signedDistribution(m_random) * 1.8f, signedDistribution(m_random) * 1.8f };
    particle.lifetime = 2.8f + unitDistribution(m_random) * 2.5f;
    particle.size = 0.72f + unitDistribution(m_random) * 0.60f;
    particle.phase = unitDistribution(m_random) * Pi * 2.0f;
    m_smoke.push_back(particle);
}

void Overlay_SmokeFire::updateSimulation(float deltaTime)
{
    const float dt = std::clamp(deltaTime, 0.0f, 0.05f);
    if (dt <= 0.0f || m_topography.empty())
    {
        return;
    }

    std::vector<cv::Point2f> spreadPositions;
    spreadPositions.reserve(m_fires.size());
    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);

    for (Fire & fire : m_fires)
    {
        fire.age += dt;
        const float intensity = fireIntensity(fire);

        fire.flameAccumulator += dt * intensity * (22.0f + m_fireSize * 1.8f);
        while (fire.flameAccumulator >= 1.0f)
        {
            spawnFlame(fire);
            fire.flameAccumulator -= 1.0f;
        }

        fire.smokeAccumulator += dt * m_smokeAmount * (4.0f + 9.0f * intensity);
        while (fire.smokeAccumulator >= 1.0f)
        {
            spawnSmoke(fire);
            fire.smokeAccumulator -= 1.0f;
        }

        if (m_spreadRate > 0.0f && intensity > 0.35f)
        {
            fire.spreadTimer -= dt * m_spreadRate;
            if (fire.spreadTimer <= 0.0f)
            {
                const float angle = unitDistribution(m_random) * Pi * 2.0f;
                const float distance = m_fireSize * (0.85f + unitDistribution(m_random) * 1.25f);
                spreadPositions.push_back({
                    fire.position.x + std::cos(angle) * distance,
                    fire.position.y + std::sin(angle) * distance });
                fire.spreadTimer = 0.55f + unitDistribution(m_random) * 1.15f;
            }
        }
    }

    std::erase_if(m_fires, [&](const Fire & fire)
    {
        return fire.age >= fire.lifetime || !isBurnable(fire.position);
    });
    for (const cv::Point2f & position : spreadPositions)
    {
        ignite(position);
    }

    const float terrainScale = (float)std::min(m_topography.cols, m_topography.rows);
    for (FlameParticle & particle : m_flames)
    {
        particle.age += dt;
        particle.phase = std::fmod(particle.phase + dt * 17.0f, Pi * 2.0f);
        const float windScale = terrainScale * 0.018f;
        particle.position.x += (particle.velocity.x + m_windX * windScale) * dt;
        particle.position.y += (particle.velocity.y + m_windY * windScale) * dt;
        particle.position.x += std::sin(particle.phase) * terrainScale * 0.0045f * dt;
        particle.velocity.x *= std::exp(-2.2f * dt);
        particle.velocity.y *= std::exp(-2.2f * dt);
        particle.altitude += particle.riseSpeed * dt;
    }
    std::erase_if(m_flames, [](const FlameParticle & particle)
    {
        return particle.age >= particle.lifetime;
    });

    for (Smoke & particle : m_smoke)
    {
        particle.age += dt;
        particle.phase = std::fmod(particle.phase + dt * 1.8f, Pi * 2.0f);
        const float life = std::clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
        const float windScale = terrainScale * 0.055f;
        particle.position.x += (particle.velocity.x + m_windX * windScale) * dt;
        particle.position.y += (particle.velocity.y + m_windY * windScale) * dt;
        particle.position.x += std::sin(particle.phase) * terrainScale * 0.006f * dt;
        particle.velocity.x *= std::exp(-0.45f * dt);
        particle.velocity.y *= std::exp(-0.45f * dt);
        particle.altitude += (0.085f + 0.055f * (1.0f - life)) * m_buoyancy * dt;
    }
    std::erase_if(m_smoke, [&](const Smoke & particle)
    {
        const float margin = terrainScale * 0.12f;
        return particle.age >= particle.lifetime
            || particle.position.x < -margin || particle.position.y < -margin
            || particle.position.x >= m_topography.cols + margin
            || particle.position.y >= m_topography.rows + margin;
    });
}

void Overlay_SmokeFire::renderFlameParticles(
    sf::RenderWindow & window,
    TopographyProcessor & processor) const
{
    if (m_flames.empty())
    {
        return;
    }

    SandBoxProjector & projector = processor.projector();
    const cv::Mat projection = projector.getProjectionMatrix();
    const float scale = projector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    std::vector<cv::Point2f> points;
    points.reserve(m_flames.size());
    for (const FlameParticle & particle : m_flames)
    {
        points.push_back(particle.position);
    }
    cv::perspectiveTransform(points, points, projection);

    const sf::Vector2f origin = projector.getTransformedPosition();
    sf::VertexArray particles(sf::PrimitiveType::Triangles);
    for (size_t i = 0; i < m_flames.size(); i++)
    {
        if (!std::isfinite(points[i].x) || !std::isfinite(points[i].y))
        {
            continue;
        }

        const FlameParticle & particle = m_flames[i];
        const float life = std::clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
        const float terrainHeight = sampleHeight(particle.position);
        const float clearance = std::max(
            0.0f,
            particle.altitude - (std::isfinite(terrainHeight) ? terrainHeight : 0.0f));
        const sf::Vector2f position(
            origin.x + points[i].x * scale,
            origin.y + points[i].y * scale - clearance * 105.0f * scale);
        const float flicker = std::clamp(
            0.68f + std::sin(particle.phase + particle.age * 31.0f) * 0.24f
                + std::sin(particle.phase * 2.3f - particle.age * 19.0f) * 0.10f,
            0.22f,
            1.0f);
        const float size = particle.size * (1.0f - life * 0.62f) * (0.78f + flicker * 0.28f);
        const float halfSize = std::max(0.7f, size * 0.5f);
        const float cooling = std::clamp((life - 0.45f) / 0.55f, 0.0f, 1.0f);
        sf::Color color(
            (std::uint8_t)(particle.color.r + (190 - particle.color.r) * cooling),
            (std::uint8_t)(particle.color.g + (25 - particle.color.g) * cooling),
            (std::uint8_t)(particle.color.b * (1.0f - cooling)),
            (std::uint8_t)(255.0f * (1.0f - life) * flicker));

        particles.append(sf::Vertex(
            { position.x - halfSize, position.y + halfSize },
            color));
        particles.append(sf::Vertex(
            { position.x + halfSize, position.y + halfSize },
            color));
        particles.append(sf::Vertex(
            { position.x + halfSize, position.y - halfSize },
            color));
        particles.append(sf::Vertex(
            { position.x - halfSize, position.y + halfSize },
            color));
        particles.append(sf::Vertex(
            { position.x + halfSize, position.y - halfSize },
            color));
        particles.append(sf::Vertex(
            { position.x - halfSize, position.y - halfSize },
            color));
    }
    window.draw(particles, sf::BlendAdd);
}

void Overlay_SmokeFire::renderSmoke(
    sf::RenderWindow & window,
    TopographyProcessor & processor) const
{
    if (m_smoke.empty())
    {
        return;
    }

    SandBoxProjector & projector = processor.projector();
    const cv::Mat projection = projector.getProjectionMatrix();
    const float scale = projector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    std::vector<cv::Point2f> points;
    points.reserve(m_smoke.size());
    for (const Smoke & particle : m_smoke)
    {
        points.push_back(particle.position);
    }
    cv::perspectiveTransform(points, points, projection);

    const sf::Vector2f origin = projector.getTransformedPosition();
    for (size_t i = 0; i < m_smoke.size(); i++)
    {
        if (!std::isfinite(points[i].x) || !std::isfinite(points[i].y))
        {
            continue;
        }

        const Smoke & particle = m_smoke[i];
        const float life = std::clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
        const float terrainHeight = sampleHeight(particle.position);
        const float clearance = std::max(
            0.0f,
            particle.altitude - (std::isfinite(terrainHeight) ? terrainHeight : 0.0f));
        const sf::Vector2f position(
            origin.x + points[i].x * scale,
            origin.y + points[i].y * scale - clearance * 120.0f * scale);
        const float radius = particle.size * (5.5f + life * 14.0f);
        const std::uint8_t alpha = (std::uint8_t)(105.0f * std::pow(1.0f - life, 1.25f));
        const std::uint8_t shade = (std::uint8_t)(58 + life * 72);

        sf::CircleShape puff(radius, 24);
        puff.setOrigin({ radius, radius });
        puff.setPosition(position);
        puff.setScale({ 1.22f, 0.86f });
        puff.setFillColor(sf::Color(shade, shade, shade, alpha));
        window.draw(puff, sf::BlendAlpha);
    }
}

void Overlay_SmokeFire::resetSimulation()
{
    m_fires.clear();
    m_flames.clear();
    m_smoke.clear();
}

void Overlay_SmokeFire::initOverlay()
{
    resetSimulation();
}

void Overlay_SmokeFire::imguiOverlay()
{
    PROFILE_FUNCTION();

    ImGui::Text(
        "Fires: %d   Flames: %d   Smoke: %d",
        (int)m_fires.size(),
        (int)m_flames.size(),
        (int)m_smoke.size());
    ImGui::SliderFloat("Fire Size", &m_fireSize, 6.0f, 36.0f, "%.0f px");
    ImGui::SliderFloat("Fire Lifetime", &m_fireLifetime, 2.0f, 20.0f, "%.1f sec");
    ImGui::SliderFloat("Spread Rate", &m_spreadRate, 0.0f, 2.5f, "%.2fx");
    ImGui::SliderFloat("Smoke Amount", &m_smokeAmount, 0.0f, 2.5f, "%.2fx");
    ImGui::SliderFloat("Smoke Buoyancy", &m_buoyancy, 0.0f, 3.0f, "%.2fx");
    ImGui::SliderFloat("Wind X", &m_windX, -1.5f, 1.5f);
    ImGui::SliderFloat("Wind Y", &m_windY, -1.5f, 1.5f);
    if (ImGui::Button("Extinguish All"))
    {
        resetSimulation();
    }
    ImGui::TextUnformatted("Left mouse: ignite terrain");
}

void Overlay_SmokeFire::processTopographyOverlay(
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
        for (Fire & fire : m_fires)
        {
            fire.position.x *= xScale;
            fire.position.y *= yScale;
        }
        for (Smoke & particle : m_smoke)
        {
            particle.position.x *= xScale;
            particle.position.y *= yScale;
            particle.velocity.x *= xScale;
            particle.velocity.y *= yScale;
        }
        for (FlameParticle & particle : m_flames)
        {
            particle.position.x *= xScale;
            particle.position.y *= yScale;
            particle.velocity.x *= xScale;
            particle.velocity.y *= yScale;
        }
    }
    m_topography = data.topography;
    m_topographySize = data.topography.size();
    updateSimulation(data.deltaTime);
}

void Overlay_SmokeFire::renderOverlay(
    sf::RenderWindow & window,
    TopographyProcessor & processor)
{
    m_processor = &processor;
    renderFlameParticles(window, processor);
    renderSmoke(window, processor);
}

void Overlay_SmokeFire::processOverlayEvent(
    const sf::Event & event,
    const sf::Vector2f & mouse,
    TopographyProcessor & processor)
{
    m_processor = &processor;
    const bool draggingProjection = processor.projector().processEvent(event, mouse);
    const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>();
    if (!mousePressed
        || mousePressed->button != sf::Mouse::Button::Left
        || draggingProjection || ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    cv::Point2f terrainPosition;
    if (mapMouseToTerrain(mouse, processor, terrainPosition))
    {
        ignite(terrainPosition);
    }
}

void Overlay_SmokeFire::saveOverlay(Settings & save) const
{
    Settings::json & settings = save.section("Overlay_SmokeFire");
    settings["m_fireSize"] = m_fireSize;
    settings["m_fireLifetime"] = m_fireLifetime;
    settings["m_spreadRate"] = m_spreadRate;
    settings["m_smokeAmount"] = m_smokeAmount;
    settings["m_buoyancy"] = m_buoyancy;
    settings["m_windX"] = m_windX;
    settings["m_windY"] = m_windY;
}

void Overlay_SmokeFire::loadOverlay(const Settings & save)
{
    const Settings::json & settings = save.section("Overlay_SmokeFire");
    Settings::read(settings, "m_fireSize", m_fireSize);
    Settings::read(settings, "m_fireLifetime", m_fireLifetime);
    Settings::read(settings, "m_spreadRate", m_spreadRate);
    Settings::read(settings, "m_smokeAmount", m_smokeAmount);
    Settings::read(settings, "m_buoyancy", m_buoyancy);
    Settings::read(settings, "m_windX", m_windX);
    Settings::read(settings, "m_windY", m_windY);
    resetSimulation();
}
