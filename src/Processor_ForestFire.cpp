#include "Processor_ForestFire.h"

#include "Profiler.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
    constexpr const char * ForestFireShaderPath = "shaders/shader_forest_fire.frag";
    constexpr float SimulationStep = 0.08f;
    constexpr int MaximumSimulationStepsPerFrame = 6;
    constexpr size_t MaximumFireParticles = 2400;
    constexpr float Pi = 3.14159265358979323846f;

    bool isValidTerrainHeight(float height)
    {
        return std::isfinite(height) && height > 0.001f && height < 0.999f;
    }
}

void Processor_ForestFire::init()
{
    reloadShader();
}

void Processor_ForestFire::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(ForestFireShaderPath, sf::Shader::Type::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Processor_ForestFire::updateBurnableMask(const cv::Mat & terrain)
{
    m_burnableMask = cv::Mat::zeros(terrain.size(), CV_8U);
    const auto heightAt = [&](int x, int y, float fallback)
    {
        x = std::clamp(x, 0, terrain.cols - 1);
        y = std::clamp(y, 0, terrain.rows - 1);
        const float height = terrain.at<float>(y, x);
        return isValidTerrainHeight(height) ? height : fallback;
    };

    for (int y = 0; y < terrain.rows; y++)
    {
        for (int x = 0; x < terrain.cols; x++)
        {
            const float height = terrain.at<float>(y, x);
            if (!isValidTerrainHeight(height) || height <= m_waterLevel || height >= m_rockLevel)
            {
                continue;
            }

            const float left = heightAt(x - 1, y, height);
            const float right = heightAt(x + 1, y, height);
            const float up = heightAt(x, y - 1, height);
            const float down = heightAt(x, y + 1, height);
            const float gradientX = (left - right) * 18.0f;
            const float gradientY = (up - down) * 18.0f;
            const float gradientSquared = gradientX * gradientX + gradientY * gradientY;
            const float steepness = std::sqrt(
                gradientSquared / (gradientSquared + 1.0f));
            if (steepness < m_rockSlope)
            {
                m_burnableMask.at<uint8_t>(y, x) = 255;
            }
        }
    }

    if (!m_fuel.empty() && m_fuel.size() == terrain.size())
    {
        for (int y = 0; y < terrain.rows; y++)
        {
            for (int x = 0; x < terrain.cols; x++)
            {
                if (m_burnableMask.at<uint8_t>(y, x) == 0)
                {
                    m_fuel.at<float>(y, x) = 0.0f;
                    m_fire.at<float>(y, x) = 0.0f;
                }
            }
        }
    }
}

void Processor_ForestFire::initializeForest(const cv::Mat & terrain)
{
    updateBurnableMask(terrain);
    m_initialFuel = cv::Mat::zeros(terrain.size(), CV_32F);
    m_fuel = cv::Mat::zeros(terrain.size(), CV_32F);
    m_fire = cv::Mat::zeros(terrain.size(), CV_32F);
    m_simulationAccumulator = 0.0f;
    m_particleSpawnAccumulator = 0.0f;
    m_burningPositions.clear();
    m_fireParticles.clear();
    m_hasLastTreePaintPosition = false;
}

void Processor_ForestFire::simulateStep(const cv::Mat & terrain, float deltaTime)
{
    if (m_fuel.empty() || m_fire.empty())
    {
        return;
    }

    cv::Mat nextFuel = m_fuel.clone();
    cv::Mat nextFire = cv::Mat::zeros(m_fire.size(), CV_32F);
    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);
    const float windLength = std::sqrt(m_windX * m_windX + m_windY * m_windY);
    const float normalizedWindX = windLength > 0.0001f ? m_windX / windLength : 0.0f;
    const float normalizedWindY = windLength > 0.0001f ? m_windY / windLength : 0.0f;
    const float windStrength = std::min(windLength, 2.0f);

    for (int y = 0; y < terrain.rows; y++)
    {
        for (int x = 0; x < terrain.cols; x++)
        {
            if (m_burnableMask.at<uint8_t>(y, x) == 0)
            {
                nextFuel.at<float>(y, x) = 0.0f;
                continue;
            }

            const float fuel = m_fuel.at<float>(y, x);
            const float fire = m_fire.at<float>(y, x);
            if (fire > 0.01f && fuel > 0.0f)
            {
                const float remainingFuel = std::max(0.0f, fuel - m_burnRate * deltaTime);
                nextFuel.at<float>(y, x) = remainingFuel;
                nextFire.at<float>(y, x) = remainingFuel > 0.0f ? 1.0f : 0.0f;
                continue;
            }
            if (fuel <= 0.02f)
            {
                continue;
            }

            float heat = 0.0f;
            const float targetHeight = terrain.at<float>(y, x);
            for (int offsetY = -1; offsetY <= 1; offsetY++)
            {
                for (int offsetX = -1; offsetX <= 1; offsetX++)
                {
                    if (offsetX == 0 && offsetY == 0)
                    {
                        continue;
                    }
                    const int neighborX = x + offsetX;
                    const int neighborY = y + offsetY;
                    if (neighborX < 0 || neighborY < 0
                        || neighborX >= terrain.cols || neighborY >= terrain.rows)
                    {
                        continue;
                    }

                    const float neighborFire = m_fire.at<float>(neighborY, neighborX);
                    if (neighborFire <= 0.01f)
                    {
                        continue;
                    }

                    const float distance = offsetX != 0 && offsetY != 0 ? 1.41421356f : 1.0f;
                    const float directionX = -offsetX / distance;
                    const float directionY = -offsetY / distance;
                    const float windAlignment = directionX * normalizedWindX
                        + directionY * normalizedWindY;
                    const float windFactor = std::max(
                        0.18f,
                        1.0f + windAlignment * windStrength * 0.85f);
                    const float neighborHeight = terrain.at<float>(neighborY, neighborX);
                    const float uphillFactor = 1.0f
                        + std::max(targetHeight - neighborHeight, 0.0f) * 2.5f;
                    heat += neighborFire * windFactor * uphillFactor / distance;
                }
            }

            const float ignitionProbability = 1.0f
                - std::exp(-std::max(m_spreadRate, 0.0f) * heat * deltaTime);
            if (unitDistribution(m_random) < ignitionProbability)
            {
                nextFire.at<float>(y, x) = 1.0f;
            }
        }
    }

    m_fuel = std::move(nextFuel);
    m_fire = std::move(nextFire);
}

void Processor_ForestFire::spawnFireParticle()
{
    if (m_burningPositions.empty() || m_fireParticles.size() >= MaximumFireParticles)
    {
        return;
    }

    std::uniform_int_distribution<size_t> burningCellDistribution(
        0, m_burningPositions.size() - 1);
    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);
    std::uniform_real_distribution<float> signedDistribution(-1.0f, 1.0f);

    FireParticle particle;
    particle.position = m_burningPositions[burningCellDistribution(m_random)];
    particle.position.x += signedDistribution(m_random) * 0.75f;
    particle.position.y += signedDistribution(m_random) * 0.75f;
    particle.velocity = {
        m_windX * 0.85f + signedDistribution(m_random) * 0.75f,
        m_windY * 0.85f + signedDistribution(m_random) * 0.55f };
    particle.lifetime = 0.32f + unitDistribution(m_random) * 0.64f;
    particle.size = 2.0f + unitDistribution(m_random) * 3.8f;
    particle.riseDistance = 5.0f + unitDistribution(m_random) * 11.0f;
    particle.phase = unitDistribution(m_random) * Pi * 2.0f;

    const float heat = unitDistribution(m_random);
    const float variation = unitDistribution(m_random);
    if (heat < 0.22f)
    {
        particle.color = sf::Color(
            255, 235, (std::uint8_t)(115.0f + variation * 95.0f));
    }
    else if (heat < 0.55f)
    {
        particle.color = sf::Color(
            255,
            (std::uint8_t)(145.0f + variation * 80.0f),
            (std::uint8_t)(12.0f + variation * 38.0f));
    }
    else if (heat < 0.84f)
    {
        particle.color = sf::Color(
            255,
            (std::uint8_t)(65.0f + variation * 85.0f),
            (std::uint8_t)(variation * 14.0f));
    }
    else
    {
        particle.color = sf::Color(
            (std::uint8_t)(205.0f + variation * 50.0f),
            (std::uint8_t)(24.0f + variation * 46.0f),
            1);
    }
    m_fireParticles.push_back(particle);
}

void Processor_ForestFire::updateFireParticles(float deltaTime)
{
    const float dt = std::clamp(deltaTime, 0.0f, 0.05f);
    if (dt <= 0.0f)
    {
        return;
    }

    if (!m_burningPositions.empty())
    {
        const float spawnRate = std::min(
            1400.0f,
            24.0f + (float)m_burningPositions.size() * 5.0f);
        m_particleSpawnAccumulator += dt * spawnRate;
        while (m_particleSpawnAccumulator >= 1.0f
            && m_fireParticles.size() < MaximumFireParticles)
        {
            spawnFireParticle();
            m_particleSpawnAccumulator -= 1.0f;
        }
    }
    else
    {
        m_particleSpawnAccumulator = 0.0f;
    }

    for (FireParticle & particle : m_fireParticles)
    {
        particle.age += dt;
        particle.phase = std::fmod(particle.phase + dt * 18.0f, Pi * 2.0f);
        particle.position.x += particle.velocity.x * dt;
        particle.position.y += particle.velocity.y * dt;
        particle.velocity.x *= std::exp(-1.8f * dt);
        particle.velocity.y *= std::exp(-1.8f * dt);
    }
    std::erase_if(m_fireParticles, [](const FireParticle & particle)
    {
        return particle.age >= particle.lifetime;
    });
}

void Processor_ForestFire::renderFireParticles(sf::RenderWindow & window)
{
    if (m_fireParticles.empty())
    {
        return;
    }

    const cv::Mat projection = m_projector.getProjectionMatrix();
    const float scale = m_projector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    std::vector<cv::Point2f> positions;
    positions.reserve(m_fireParticles.size());
    for (const FireParticle & particle : m_fireParticles)
    {
        positions.push_back(particle.position);
    }
    cv::perspectiveTransform(positions, positions, projection);

    const sf::Vector2f origin = m_projector.getTransformedPosition();
    sf::VertexArray particles(sf::PrimitiveType::Triangles);
    for (size_t i = 0; i < m_fireParticles.size(); i++)
    {
        if (!std::isfinite(positions[i].x) || !std::isfinite(positions[i].y))
        {
            continue;
        }

        const FireParticle & particle = m_fireParticles[i];
        const float life = std::clamp(particle.age / particle.lifetime, 0.0f, 1.0f);
        const float flicker = std::clamp(
            0.76f + std::sin(particle.phase + particle.age * 29.0f) * 0.24f,
            0.35f,
            1.0f);
        const sf::Vector2f position(
            origin.x + positions[i].x * scale
                + std::sin(particle.phase) * 1.2f,
            origin.y + positions[i].y * scale
                - life * particle.riseDistance);
        const float halfSize = std::max(
            0.7f,
            particle.size * (1.0f - life * 0.58f) * flicker * 0.5f);
        const float cooling = std::clamp((life - 0.42f) / 0.58f, 0.0f, 1.0f);
        const sf::Color color(
            (std::uint8_t)(particle.color.r + (190 - particle.color.r) * cooling),
            (std::uint8_t)(particle.color.g + (24 - particle.color.g) * cooling),
            (std::uint8_t)(particle.color.b * (1.0f - cooling)),
            (std::uint8_t)(255.0f * (1.0f - life) * flicker));

        particles.append(sf::Vertex(
            { position.x - halfSize, position.y + halfSize }, color));
        particles.append(sf::Vertex(
            { position.x + halfSize, position.y + halfSize }, color));
        particles.append(sf::Vertex(
            { position.x + halfSize, position.y - halfSize }, color));
        particles.append(sf::Vertex(
            { position.x - halfSize, position.y + halfSize }, color));
        particles.append(sf::Vertex(
            { position.x + halfSize, position.y - halfSize }, color));
        particles.append(sf::Vertex(
            { position.x - halfSize, position.y - halfSize }, color));
    }
    window.draw(particles, sf::BlendAdd);
}

void Processor_ForestFire::applyTreeBrush(
    const cv::Point2f & position,
    float direction)
{
    if (m_initialFuel.empty() || m_fuel.empty() || m_fire.empty())
    {
        return;
    }

    const float radius = std::max(m_treeBrushSize, 1.0f);
    const float sigma = std::max(m_treeBrushBlur, 0.1f);
    const float radiusSquared = radius * radius;
    const float gaussianDenominator = 2.0f * sigma * sigma;
    const int minimumX = std::max(0, (int)std::floor(position.x - radius));
    const int maximumX = std::min(m_fuel.cols - 1, (int)std::ceil(position.x + radius));
    const int minimumY = std::max(0, (int)std::floor(position.y - radius));
    const int maximumY = std::min(m_fuel.rows - 1, (int)std::ceil(position.y + radius));

    for (int y = minimumY; y <= maximumY; y++)
    {
        for (int x = minimumX; x <= maximumX; x++)
        {
            if (m_burnableMask.at<uint8_t>(y, x) == 0)
            {
                continue;
            }

            const float dx = x - position.x;
            const float dy = y - position.y;
            const float distanceSquared = dx * dx + dy * dy;
            if (distanceSquared > radiusSquared)
            {
                continue;
            }

            const float weight = std::exp(-distanceSquared / gaussianDenominator);
            const float amount = direction * m_treePaintAmount * weight;
            float & initialFuel = m_initialFuel.at<float>(y, x);
            float & fuel = m_fuel.at<float>(y, x);
            initialFuel = std::clamp(initialFuel + amount, 0.0f, 1.0f);
            fuel = std::clamp(fuel + amount, 0.0f, initialFuel);
            if (direction < 0.0f && fuel <= 0.02f)
            {
                fuel = 0.0f;
                m_fire.at<float>(y, x) = 0.0f;
            }
        }
    }
}

void Processor_ForestFire::paintTreeLine(
    const cv::Point2f & from,
    const cv::Point2f & to,
    float direction)
{
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    const float spacing = std::max(1.0f, m_treeBrushSize * 0.25f);
    const int steps = std::max(1, (int)std::ceil(distance / spacing));
    for (int step = 1; step <= steps; step++)
    {
        const float amount = (float)step / steps;
        applyTreeBrush(
            { from.x + dx * amount, from.y + dy * amount },
            direction);
    }
}

void Processor_ForestFire::ignite(const cv::Point2f & position, float radius)
{
    if (m_fuel.empty() || m_fire.empty())
    {
        return;
    }

    const int minimumX = std::max(0, (int)std::floor(position.x - radius));
    const int maximumX = std::min(m_fire.cols - 1, (int)std::ceil(position.x + radius));
    const int minimumY = std::max(0, (int)std::floor(position.y - radius));
    const int maximumY = std::min(m_fire.rows - 1, (int)std::ceil(position.y + radius));
    const float radiusSquared = radius * radius;
    for (int y = minimumY; y <= maximumY; y++)
    {
        for (int x = minimumX; x <= maximumX; x++)
        {
            const float dx = x - position.x;
            const float dy = y - position.y;
            if (dx * dx + dy * dy <= radiusSquared
                && m_burnableMask.at<uint8_t>(y, x) != 0
                && m_fuel.at<float>(y, x) > 0.02f)
            {
                m_fire.at<float>(y, x) = 1.0f;
            }
        }
    }
}

bool Processor_ForestFire::igniteRandomFire()
{
    if (m_fuel.empty())
    {
        return false;
    }

    std::uniform_int_distribution<int> xDistribution(0, m_fuel.cols - 1);
    std::uniform_int_distribution<int> yDistribution(0, m_fuel.rows - 1);
    for (int attempt = 0; attempt < 2048; attempt++)
    {
        const int x = xDistribution(m_random);
        const int y = yDistribution(m_random);
        if (m_fuel.at<float>(y, x) > 0.20f)
        {
            ignite({ (float)x, (float)y }, m_ignitionRadius);
            return true;
        }
    }

    for (int y = 0; y < m_fuel.rows; y++)
    {
        for (int x = 0; x < m_fuel.cols; x++)
        {
            if (m_fuel.at<float>(y, x) > 0.02f)
            {
                ignite({ (float)x, (float)y }, m_ignitionRadius);
                return true;
            }
        }
    }
    return false;
}

void Processor_ForestFire::extinguish()
{
    if (!m_fire.empty())
    {
        m_fire.setTo(0.0f);
    }
    m_particleSpawnAccumulator = 0.0f;
    m_fireParticles.clear();
}

void Processor_ForestFire::updateStatistics()
{
    m_burningCells = 0;
    m_treeCells = 0;
    m_burningPositions.clear();
    double currentFuel = 0.0;
    double initialFuel = 0.0;
    for (int y = 0; y < m_fuel.rows; y++)
    {
        for (int x = 0; x < m_fuel.cols; x++)
        {
            const float initial = m_initialFuel.at<float>(y, x);
            if (initial > 0.02f)
            {
                m_treeCells++;
                initialFuel += initial;
                currentFuel += m_fuel.at<float>(y, x);
            }
            if (m_fire.at<float>(y, x) > 0.01f)
            {
                m_burningCells++;
                m_burningPositions.push_back({ (float)x, (float)y });
            }
        }
    }
    m_fuelRemaining = initialFuel > 0.0
        ? (float)std::clamp(currentFuel / initialFuel, 0.0, 1.0)
        : 0.0f;
}

void Processor_ForestFire::updateTexture(const cv::Mat & terrain)
{
    cv::Mat state(terrain.size(), CV_32FC4);
    for (int y = 0; y < terrain.rows; y++)
    {
        for (int x = 0; x < terrain.cols; x++)
        {
            state.at<cv::Vec4f>(y, x) = {
                terrain.at<float>(y, x),
                m_fuel.at<float>(y, x),
                m_fire.at<float>(y, x),
                m_initialFuel.at<float>(y, x) };
        }
    }

    m_projector.project(state, m_projectedState);
    if (m_projectedState.empty())
    {
        m_hasFrame = false;
        return;
    }

    cv::Mat rgba;
    m_projectedState.convertTo(rgba, CV_8UC4, 255.0);
    m_image.resize({ (unsigned int)rgba.cols, (unsigned int)rgba.rows }, rgba.ptr());
    if (!m_texture.loadFromImage(m_image))
    {
        std::cerr << "Failed to load the forest-fire terrain texture.\n";
        return;
    }
    m_texture.setSmooth(true);
    m_sprite.setTexture(m_texture, true);
    m_hasFrame = true;
}

bool Processor_ForestFire::mapMouseToTerrain(
    const sf::Vector2f & mouse,
    cv::Point2f & terrainPosition)
{
    if (m_topography.empty())
    {
        return false;
    }

    const float scale = m_projector.getTransformedScale();
    if (!std::isfinite(scale) || scale <= 0.0f)
    {
        return false;
    }

    const sf::Vector2f offset = mouse - m_projector.getTransformedPosition();
    std::vector<cv::Point2f> point = { { offset.x / scale, offset.y / scale } };
    const cv::Mat projection = m_projector.getProjectionMatrix();
    if (projection.empty())
    {
        return false;
    }

    cv::Mat inverseProjection;
    if (cv::invert(projection, inverseProjection) == 0.0)
    {
        return false;
    }
    cv::perspectiveTransform(point, point, inverseProjection);
    terrainPosition = point.front();
    return terrainPosition.x >= 0.0f && terrainPosition.y >= 0.0f
        && terrainPosition.x < m_topography.cols
        && terrainPosition.y < m_topography.rows;
}

void Processor_ForestFire::imgui()
{
    PROFILE_FUNCTION();

    bool clearTrees = false;
    clearTrees |= ImGui::SliderFloat("Water Level", &m_waterLevel, 0.05f, 0.60f);
    clearTrees |= ImGui::SliderFloat("Rock Level", &m_rockLevel, 0.35f, 0.95f);
    m_rockLevel = std::max(m_rockLevel, m_waterLevel + 0.10f);
    ImGui::SliderFloat("Tree Brush Size", &m_treeBrushSize, 1.0f, 128.0f, "%.0f px");
    ImGui::SliderFloat("Tree Brush Blur", &m_treeBrushBlur, 0.5f, 64.0f, "%.1f");
    ImGui::SliderFloat("Tree Paint Amount", &m_treePaintAmount, 0.005f, 0.25f, "%.3f");
    ImGui::SliderFloat("Spread Rate", &m_spreadRate, 0.0f, 3.0f, "%.2f");
    ImGui::SliderFloat("Burn Rate", &m_burnRate, 0.02f, 0.60f, "%.2f fuel/sec");
    ImGui::SliderFloat("Ignition Radius", &m_ignitionRadius, 1.0f, 30.0f, "%.0f px");
    ImGui::SliderFloat("Wind X", &m_windX, -2.0f, 2.0f);
    ImGui::SliderFloat("Wind Y", &m_windY, -2.0f, 2.0f);
    ImGui::Checkbox("Paused", &m_paused);
    if (clearTrees)
    {
        m_resetRequested = true;
    }

    ImGui::Text("Burning Cells: %d", m_burningCells);
    ImGui::Text("Forested Cells: %d", m_treeCells);
    ImGui::Text("Fuel Remaining: %.1f%%", m_fuelRemaining * 100.0f);
    ImGui::TextUnformatted("Left mouse: paint trees");
    ImGui::TextUnformatted("Middle mouse: remove trees");
    ImGui::TextUnformatted("Right mouse: ignite forest");

    if (ImGui::Button("Ignite Random Fire"))
    {
        igniteRandomFire();
    }
    ImGui::SameLine();
    if (ImGui::Button("Extinguish"))
    {
        extinguish();
    }
    if (ImGui::Button("Reset / Clear Trees"))
    {
        m_resetRequested = true;
    }
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }

    ImGui::Separator();
    m_projector.imgui();
}

void Processor_ForestFire::render(sf::RenderWindow & window)
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
            1.0f / std::max(textureSize.x, 1u),
            1.0f / std::max(textureSize.y, 1u)));
        m_shader.setUniform("waterLevel", m_waterLevel);
        m_shader.setUniform("rockLevel", m_rockLevel);
        m_shader.setUniform("rockSlope", m_rockSlope);
        window.draw(m_sprite, &m_shader);
    }
    else
    {
        window.draw(m_sprite);
    }
    renderFireParticles(window);
}

void Processor_ForestFire::processEvent(
    const sf::Event & event,
    const sf::Vector2f & mouse)
{
    const bool draggingProjection = m_projector.processEvent(event, mouse);
    if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>();
        mouseReleased
        && (mouseReleased->button == sf::Mouse::Button::Left
            || mouseReleased->button == sf::Mouse::Button::Middle))
    {
        m_hasLastTreePaintPosition = false;
        return;
    }

    if (draggingProjection || ImGui::GetIO().WantCaptureMouse)
    {
        m_hasLastTreePaintPosition = false;
        return;
    }

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>();
        mousePressed && mousePressed->button == sf::Mouse::Button::Right)
    {
        cv::Point2f terrainPosition;
        if (mapMouseToTerrain(mouse, terrainPosition))
        {
            ignite(terrainPosition, m_ignitionRadius);
        }
        return;
    }

    float paintDirection = 0.0f;
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
    {
        paintDirection = 1.0f;
    }
    else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
    {
        paintDirection = -1.0f;
    }

    if (paintDirection == 0.0f)
    {
        if (event.is<sf::Event::MouseMoved>())
        {
            m_hasLastTreePaintPosition = false;
        }
        return;
    }

    cv::Point2f terrainPosition;
    if (!mapMouseToTerrain(mouse, terrainPosition))
    {
        m_hasLastTreePaintPosition = false;
        return;
    }

    if (event.is<sf::Event::MouseButtonPressed>())
    {
        applyTreeBrush(terrainPosition, paintDirection);
        m_lastTreePaintPosition = terrainPosition;
        m_hasLastTreePaintPosition = true;
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        if (m_hasLastTreePaintPosition)
        {
            paintTreeLine(m_lastTreePaintPosition, terrainPosition, paintDirection);
        }
        else
        {
            applyTreeBrush(terrainPosition, paintDirection);
        }
        m_lastTreePaintPosition = terrainPosition;
        m_hasLastTreePaintPosition = true;
    }
}

void Processor_ForestFire::save(Save & save) const
{
    Save::Json & settings = save.section("Processor_ForestFire");
    settings["m_waterLevel"] = m_waterLevel;
    settings["m_rockLevel"] = m_rockLevel;
    settings["m_treeBrushSize"] = m_treeBrushSize;
    settings["m_treeBrushBlur"] = m_treeBrushBlur;
    settings["m_treePaintAmount"] = m_treePaintAmount;
    settings["m_spreadRate"] = m_spreadRate;
    settings["m_burnRate"] = m_burnRate;
    settings["m_windX"] = m_windX;
    settings["m_windY"] = m_windY;
    settings["m_ignitionRadius"] = m_ignitionRadius;
    settings["m_paused"] = m_paused;
    m_projector.save(save);
}

void Processor_ForestFire::load(const Save & save)
{
    const Save::Json & settings = save.section("Processor_ForestFire");
    Save::read(settings, "m_waterLevel", m_waterLevel);
    Save::read(settings, "m_rockLevel", m_rockLevel);
    Save::read(settings, "m_treeBrushSize", m_treeBrushSize);
    Save::read(settings, "m_treeBrushBlur", m_treeBrushBlur);
    Save::read(settings, "m_treePaintAmount", m_treePaintAmount);
    Save::read(settings, "m_spreadRate", m_spreadRate);
    Save::read(settings, "m_burnRate", m_burnRate);
    Save::read(settings, "m_windX", m_windX);
    Save::read(settings, "m_windY", m_windY);
    Save::read(settings, "m_ignitionRadius", m_ignitionRadius);
    Save::read(settings, "m_paused", m_paused);
    m_resetRequested = true;
    m_projector.load(save);
}

void Processor_ForestFire::onSourceChanged()
{
    m_resetRequested = true;
    m_simulationAccumulator = 0.0f;
    m_particleSpawnAccumulator = 0.0f;
    m_fireParticles.clear();
}

void Processor_ForestFire::processTopography(const IntermediateData & data)
{
    PROFILE_FUNCTION();
    if (data.topography.empty() || data.topography.type() != CV_32F)
    {
        m_hasFrame = false;
        return;
    }

    m_topography = data.topography;
    const bool sizeChanged = m_topographySize != data.topography.size();
    m_topographySize = data.topography.size();
    if (m_resetRequested || sizeChanged || m_fuel.empty())
    {
        initializeForest(m_topography);
        m_resetRequested = false;
    }
    else
    {
        updateBurnableMask(m_topography);
    }

    if (!m_paused)
    {
        m_simulationAccumulator += std::clamp(data.deltaTime, 0.0f, 0.10f);
        int steps = 0;
        while (m_simulationAccumulator >= SimulationStep
            && steps < MaximumSimulationStepsPerFrame)
        {
            simulateStep(m_topography, SimulationStep);
            m_simulationAccumulator -= SimulationStep;
            steps++;
        }
        if (steps == MaximumSimulationStepsPerFrame)
        {
            m_simulationAccumulator = std::min(m_simulationAccumulator, SimulationStep);
        }
    }

    updateStatistics();
    if (!m_paused)
    {
        updateFireParticles(data.deltaTime);
    }
    updateTexture(m_topography);
}
