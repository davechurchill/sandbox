#include "Processor_FishPond.h"

#include "Profiler.hpp"
#include "Tools.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>

namespace
{
    constexpr const char * FishPondShaderPath = "shaders/shader_fish_pond.frag";
    constexpr float Pi = 3.14159265358979323846f;
    constexpr float RadiansToDegrees = 180.0f / Pi;
    constexpr float FishBottomClearance = 0.012f;
    const std::array<sf::Color, 5> FishColors = {
        sf::Color(245, 132, 30),
        sf::Color(255, 190, 45),
        sf::Color(235, 238, 214),
        sf::Color(72, 164, 190),
        sf::Color(235, 118, 155) };

    int fishDepthBand(float depth)
    {
        return std::clamp((int)(depth * 4.0f), 0, 3);
    }

    float vectorLength(const cv::Point2f & value)
    {
        return std::sqrt(value.x * value.x + value.y * value.y);
    }

    cv::Point2f normalized(const cv::Point2f & value, const cv::Point2f & fallback = { 1.0f, 0.0f })
    {
        const float length = vectorLength(value);
        return length > 0.0001f
            ? cv::Point2f(value.x / length, value.y / length)
            : fallback;
    }

    bool isTerrainHeight(float height)
    {
        return std::isfinite(height) && height > 0.001f && height < 0.999f;
    }
}

void Processor_FishPond::init()
{
    reloadShader();
    resetFish();
}

void Processor_FishPond::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(FishPondShaderPath, sf::Shader::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

float Processor_FishPond::sampleHeight(const cv::Point2f & position) const
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

bool Processor_FishPond::isSwimmable(const cv::Point2f & position) const
{
    return isSwimmable(position, m_minimumDepth);
}

bool Processor_FishPond::isSwimmable(
    const cv::Point2f & position,
    float swimDepth) const
{
    const float height = sampleHeight(position);
    return std::isfinite(height)
        && 1.0f - height >= swimDepth + FishBottomClearance;
}

void Processor_FishPond::randomizeWander(Fish & fish)
{
    std::uniform_real_distribution<float> angleDistribution(0.0f, Pi * 2.0f);
    std::uniform_real_distribution<float> timerDistribution(0.8f, 2.8f);
    const float angle = angleDistribution(m_random);
    fish.wanderDirection = { std::cos(angle), std::sin(angle) };
    fish.wanderTimer = timerDistribution(m_random);
}

bool Processor_FishPond::addFish(const cv::Point2f & position)
{
    if ((int)m_fish.size() >= MaximumFishCount || !isSwimmable(position))
    {
        return false;
    }

    std::uniform_real_distribution<float> angleDistribution(0.0f, Pi * 2.0f);
    std::uniform_real_distribution<float> unitDistribution(0.0f, 1.0f);
    const float angle = angleDistribution(m_random);
    Fish fish;
    fish.position = position;
    fish.velocity = { std::cos(angle), std::sin(angle) };
    fish.phase = unitDistribution(m_random) * Pi * 2.0f;
    const float availableDepth = 1.0f - sampleHeight(position) - FishBottomClearance;
    const float deepestDepth = std::min(m_maximumFishDepth, availableDepth);
    std::uniform_real_distribution<float> depthDistribution(m_minimumDepth, deepestDepth);
    fish.swimDepth = depthDistribution(m_random);
    randomizeWander(fish);

    float nearestSchoolDistanceSquared = m_schoolRadius * m_schoolRadius * 2.25f;
    int nearbyColorType = -1;
    const int depthBand = fishDepthBand(fish.swimDepth);
    for (const Fish & nearbyFish : m_fish)
    {
        if (fishDepthBand(nearbyFish.swimDepth) != depthBand)
        {
            continue;
        }
        const float dx = nearbyFish.position.x - position.x;
        const float dy = nearbyFish.position.y - position.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < nearestSchoolDistanceSquared)
        {
            nearestSchoolDistanceSquared = distanceSquared;
            nearbyColorType = nearbyFish.colorType;
        }
    }
    if (nearbyColorType < 0)
    {
        std::uniform_int_distribution<int> colorDistribution(
            0,
            (int)FishColors.size() - 1);
        nearbyColorType = colorDistribution(m_random);
    }
    fish.colorType = nearbyColorType;
    const sf::Color baseColor = FishColors[fish.colorType];
    std::uniform_int_distribution<int> colorVariation(-10, 10);
    fish.color = sf::Color(
        (sf::Uint8)std::clamp((int)baseColor.r + colorVariation(m_random), 0, 255),
        (sf::Uint8)std::clamp((int)baseColor.g + colorVariation(m_random), 0, 255),
        (sf::Uint8)std::clamp((int)baseColor.b + colorVariation(m_random), 0, 255));
    m_fish.push_back(fish);
    return true;
}

bool Processor_FishPond::spawnRandomFish()
{
    if (m_topography.empty())
    {
        return false;
    }

    std::uniform_real_distribution<float> xDistribution(0.0f, std::max(0.0f, m_topography.cols - 0.001f));
    std::uniform_real_distribution<float> yDistribution(0.0f, std::max(0.0f, m_topography.rows - 0.001f));
    for (int attempt = 0; attempt < 512; attempt++)
    {
        if (addFish({ xDistribution(m_random), yDistribution(m_random) }))
        {
            return true;
        }
    }
    return false;
}

void Processor_FishPond::resetFish()
{
    m_fish.clear();
    m_resetPending = true;
}

cv::Point2f Processor_FishPond::shorelineSteering(
    const Fish & fish,
    const cv::Point2f & forward) const
{
    const float lookAhead = std::max(8.0f, m_schoolRadius * 0.30f);
    cv::Point2f steering(0.0f, 0.0f);

    static const std::array<cv::Point2f, 8> Directions = {
        cv::Point2f(1.0f, 0.0f), cv::Point2f(-1.0f, 0.0f),
        cv::Point2f(0.0f, 1.0f), cv::Point2f(0.0f, -1.0f),
        cv::Point2f(0.707f, 0.707f), cv::Point2f(-0.707f, 0.707f),
        cv::Point2f(0.707f, -0.707f), cv::Point2f(-0.707f, -0.707f) };
    for (const cv::Point2f & direction : Directions)
    {
        const cv::Point2f sample(
            fish.position.x + direction.x * lookAhead,
            fish.position.y + direction.y * lookAhead);
        if (!isSwimmable(sample, fish.swimDepth))
        {
            steering.x -= direction.x;
            steering.y -= direction.y;
        }
    }

    const cv::Point2f ahead(
        fish.position.x + forward.x * lookAhead * 1.6f,
        fish.position.y + forward.y * lookAhead * 1.6f);
    if (!isSwimmable(ahead, fish.swimDepth))
    {
        steering.x -= forward.x * 2.4f;
        steering.y -= forward.y * 2.4f;

        const cv::Point2f left(-forward.y, forward.x);
        const cv::Point2f right(forward.y, -forward.x);
        const bool leftOpen = isSwimmable({
            fish.position.x + left.x * lookAhead,
            fish.position.y + left.y * lookAhead }, fish.swimDepth);
        const bool rightOpen = isSwimmable({
            fish.position.x + right.x * lookAhead,
            fish.position.y + right.y * lookAhead }, fish.swimDepth);
        if (leftOpen != rightOpen)
        {
            const cv::Point2f turn = leftOpen ? left : right;
            steering.x += turn.x * 2.0f;
            steering.y += turn.y * 2.0f;
        }
    }
    return steering;
}

void Processor_FishPond::updateFish(float deltaTime)
{
    if (m_topography.empty())
    {
        return;
    }

    if (m_resetPending)
    {
        m_fish.clear();
        m_resetPending = false;
    }
    for (Fish & fish : m_fish)
    {
        fish.swimDepth = std::clamp(
            fish.swimDepth,
            m_minimumDepth,
            m_maximumFishDepth);
    }
    m_fish.erase(
        std::remove_if(m_fish.begin(), m_fish.end(),
            [this](const Fish & fish)
            {
                return !isSwimmable(fish.position, fish.swimDepth);
            }),
        m_fish.end());
    while ((int)m_fish.size() < m_targetFishCount)
    {
        if (!spawnRandomFish())
        {
            break;
        }
    }
    if ((int)m_fish.size() > m_targetFishCount)
    {
        m_fish.resize(m_targetFishCount);
    }

    const float dt = std::clamp(deltaTime, 0.0f, 0.05f);
    const float neighborRadiusSquared = m_schoolRadius * m_schoolRadius;
    const float separationRadius = std::max(5.0f, m_schoolRadius * 0.34f);
    const float separationRadiusSquared = separationRadius * separationRadius;
    const float baseSpeed = std::min(m_topography.cols, m_topography.rows) * 0.045f * m_speedMultiplier;

    std::vector<cv::Point2f> nextDirections(m_fish.size());
    for (size_t i = 0; i < m_fish.size(); i++)
    {
        Fish & fish = m_fish[i];
        fish.wanderTimer -= dt;
        if (fish.wanderTimer <= 0.0f)
        {
            randomizeWander(fish);
        }

        const cv::Point2f forward = normalized(fish.velocity);
        cv::Point2f center(0.0f, 0.0f);
        cv::Point2f alignment(0.0f, 0.0f);
        cv::Point2f separation(0.0f, 0.0f);
        int neighbors = 0;
        const int depthBand = fishDepthBand(fish.swimDepth);
        for (size_t j = 0; j < m_fish.size(); j++)
        {
            if (i == j)
            {
                continue;
            }
            const int neighborDepthBand = fishDepthBand(m_fish[j].swimDepth);
            if (neighborDepthBand != depthBand)
            {
                continue;
            }
            const float dx = m_fish[j].position.x - fish.position.x;
            const float dy = m_fish[j].position.y - fish.position.y;
            const float distanceSquared = dx * dx + dy * dy;
            if (distanceSquared <= 0.0001f || distanceSquared > neighborRadiusSquared)
            {
                continue;
            }

            if (distanceSquared < separationRadiusSquared)
            {
                const float inverseDistance = 1.0f / std::sqrt(distanceSquared);
                const float strength = 1.0f - std::sqrt(distanceSquared) / separationRadius;
                separation.x -= dx * inverseDistance * strength;
                separation.y -= dy * inverseDistance * strength;
            }
            if (m_fish[j].colorType != fish.colorType)
            {
                continue;
            }

            center.x += m_fish[j].position.x;
            center.y += m_fish[j].position.y;
            const cv::Point2f neighborDirection = normalized(m_fish[j].velocity);
            alignment.x += neighborDirection.x;
            alignment.y += neighborDirection.y;
            neighbors++;
        }

        cv::Point2f desired(
            forward.x * 0.62f + fish.wanderDirection.x * 0.20f,
            forward.y * 0.62f + fish.wanderDirection.y * 0.20f);
        if (neighbors > 0)
        {
            center.x = center.x / neighbors - fish.position.x;
            center.y = center.y / neighbors - fish.position.y;
            const cv::Point2f cohesion = normalized(center, forward);
            alignment = normalized(alignment, forward);
            desired.x += (cohesion.x * 0.58f + alignment.x * 0.82f) * m_schoolStrength;
            desired.y += (cohesion.y * 0.58f + alignment.y * 0.82f) * m_schoolStrength;
        }
        desired.x += separation.x * m_separationStrength;
        desired.y += separation.y * m_separationStrength;

        const cv::Point2f avoidance = shorelineSteering(fish, forward);
        desired.x += avoidance.x * 1.65f;
        desired.y += avoidance.y * 1.65f;
        desired = normalized(desired, forward);

        const float steeringAmount = std::clamp(dt * 2.8f, 0.0f, 1.0f);
        nextDirections[i] = normalized({
            forward.x + (desired.x - forward.x) * steeringAmount,
            forward.y + (desired.y - forward.y) * steeringAmount }, forward);
    }

    for (size_t i = 0; i < m_fish.size(); i++)
    {
        Fish & fish = m_fish[i];
        const float height = sampleHeight(fish.position);
        const float depth = std::isfinite(height) ? 1.0f - height : m_minimumDepth;
        const float depthSpeed = 0.42f + std::sqrt(std::clamp(depth, 0.0f, 1.0f)) * 0.58f;
        const float speed = baseSpeed * depthSpeed;
        fish.velocity = { nextDirections[i].x * speed, nextDirections[i].y * speed };
        fish.phase = std::fmod(fish.phase + dt * (5.0f + speed * 0.12f), Pi * 2.0f);

        const cv::Point2f next(
            fish.position.x + fish.velocity.x * dt,
            fish.position.y + fish.velocity.y * dt);
        if (isSwimmable(next, fish.swimDepth))
        {
            fish.position = next;
        }
        else
        {
            const cv::Point2f left(-nextDirections[i].y, nextDirections[i].x);
            fish.velocity = { left.x * speed, left.y * speed };
            fish.wanderDirection = left;
            fish.wanderTimer = 0.45f;
        }
    }
}

bool Processor_FishPond::mapMouseToTerrain(
    const sf::Vector2f & mouse,
    cv::Point2f & terrainPosition)
{
    if (m_topography.empty())
    {
        return false;
    }

    const float scale = m_projector.getTransformedScale();
    const cv::Mat projection = m_projector.getProjectionMatrix();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return false;
    }

    cv::Mat inverseProjection;
    if (cv::invert(projection, inverseProjection) == 0.0)
    {
        return false;
    }
    const sf::Vector2f local = (mouse - m_projector.getTransformedPosition()) / scale;
    std::vector<cv::Point2f> point = { { local.x, local.y } };
    cv::perspectiveTransform(point, point, inverseProjection);
    terrainPosition = point.front();
    return isSwimmable(terrainPosition);
}

void Processor_FishPond::drawFish(
    sf::RenderWindow & window,
    const sf::Vector2f & position,
    const sf::Vector2f & direction,
    const Fish & fish) const
{
    const float directionLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    const sf::Vector2f forward = directionLength > 0.001f
        ? direction / directionLength
        : sf::Vector2f(1.0f, 0.0f);
    const sf::Vector2f side(-forward.y, forward.x);
    const float heading = std::atan2(forward.y, forward.x) * RadiansToDegrees;
    const float depthRange = std::max(0.001f, m_maximumFishDepth - m_minimumDepth);
    const float visualDepth = std::clamp(
        (fish.swimDepth - m_minimumDepth) / depthRange,
        0.0f,
        1.0f);
    const float fishSize = m_fishSize * (1.0f - visualDepth * 0.24f);
    const float waterTint = visualDepth * 0.68f;
    const sf::Color bodyColor(
        (sf::Uint8)(fish.color.r * (1.0f - waterTint) + 7.0f * waterTint),
        (sf::Uint8)(fish.color.g * (1.0f - waterTint) + 24.0f * waterTint),
        (sf::Uint8)(fish.color.b * (1.0f - waterTint) + 38.0f * waterTint),
        (sf::Uint8)(255.0f - visualDepth * 72.0f));
    const float tailSwing = std::sin(fish.phase) * fishSize * 0.32f;

    sf::ConvexShape tail;
    tail.setPointCount(3);
    const sf::Vector2f tailBase = position - forward * fishSize * 0.62f;
    const sf::Vector2f tailTip = position - forward * fishSize * 1.18f + side * tailSwing;
    tail.setPoint(0, tailBase + side * fishSize * 0.42f);
    tail.setPoint(1, tailBase - side * fishSize * 0.42f);
    tail.setPoint(2, tailTip);
    tail.setFillColor(sf::Color(
        (sf::Uint8)(bodyColor.r * 0.82f),
        (sf::Uint8)(bodyColor.g * 0.82f),
        (sf::Uint8)(bodyColor.b * 0.82f),
        bodyColor.a));
    window.draw(tail);

    const float bodyRadius = fishSize * 0.50f;
    sf::CircleShape body(bodyRadius, 26);
    body.setOrigin(bodyRadius, bodyRadius);
    body.setPosition(position);
    body.setScale(1.48f, 0.70f);
    body.setRotation(heading);
    body.setFillColor(bodyColor);
    body.setOutlineColor(sf::Color(
        (sf::Uint8)(bodyColor.r * 0.48f),
        (sf::Uint8)(bodyColor.g * 0.48f),
        (sf::Uint8)(bodyColor.b * 0.48f),
        bodyColor.a));
    body.setOutlineThickness(1.0f);
    window.draw(body);

    const sf::Vector2f eyePosition = position
        + forward * fishSize * 0.47f
        + side * fishSize * 0.22f;
    sf::CircleShape eye(std::max(1.0f, fishSize * 0.085f), 12);
    eye.setOrigin(eye.getRadius(), eye.getRadius());
    eye.setPosition(eyePosition);
    eye.setFillColor(sf::Color(18, 20, 22, bodyColor.a));
    window.draw(eye);
}

void Processor_FishPond::renderFish(sf::RenderWindow & window)
{
    if (m_fish.empty())
    {
        return;
    }

    const cv::Mat projection = m_projector.getProjectionMatrix();
    const float scale = m_projector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    std::vector<cv::Point2f> points;
    points.reserve(m_fish.size() * 2);
    for (const Fish & fish : m_fish)
    {
        const cv::Point2f direction = normalized(fish.velocity);
        points.push_back(fish.position);
        points.push_back({ fish.position.x + direction.x * 4.0f, fish.position.y + direction.y * 4.0f });
    }
    cv::perspectiveTransform(points, points, projection);

    const sf::Vector2f origin = m_projector.getTransformedPosition();
    std::vector<size_t> drawOrder(m_fish.size());
    std::iota(drawOrder.begin(), drawOrder.end(), 0);
    std::stable_sort(drawOrder.begin(), drawOrder.end(),
        [this](size_t first, size_t second)
        {
            return m_fish[first].swimDepth > m_fish[second].swimDepth;
        });
    for (size_t i : drawOrder)
    {
        const cv::Point2f & point = points[i * 2];
        const cv::Point2f & ahead = points[i * 2 + 1];
        if (!std::isfinite(point.x) || !std::isfinite(point.y)
            || !std::isfinite(ahead.x) || !std::isfinite(ahead.y))
        {
            continue;
        }
        drawFish(
            window,
            { origin.x + point.x * scale, origin.y + point.y * scale },
            { ahead.x - point.x, ahead.y - point.y },
            m_fish[i]);
    }
}

void Processor_FishPond::imgui()
{
    PROFILE_FUNCTION();

    ImGui::Text("Fish: %d", (int)m_fish.size());
    ImGui::SliderInt("Fish Count", &m_targetFishCount, 0, MaximumFishCount);
    ImGui::SliderFloat("Swim Speed", &m_speedMultiplier, 0.2f, 3.0f, "%.1fx");
    ImGui::SliderFloat("Fish Size", &m_fishSize, 6.0f, 24.0f);
    ImGui::SliderFloat("School Radius", &m_schoolRadius, 12.0f, 120.0f, "%.0f px");
    ImGui::SliderFloat("School Strength", &m_schoolStrength, 0.0f, 2.5f);
    ImGui::SliderFloat("Separation Strength", &m_separationStrength, 0.0f, 3.0f);
    if (ImGui::SliderFloat("Shallowest Fish Depth", &m_minimumDepth, 0.01f, 0.35f))
    {
        m_maximumFishDepth = std::max(m_maximumFishDepth, m_minimumDepth);
    }
    if (ImGui::SliderFloat("Deepest Fish Depth", &m_maximumFishDepth, 0.05f, 0.90f))
    {
        m_maximumFishDepth = std::max(m_maximumFishDepth, m_minimumDepth);
    }
    if (ImGui::Button("Reset Fish"))
    {
        resetFish();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }
    ImGui::TextUnformatted("Left mouse: add fish");

    ImGui::Separator();
    m_projector.imgui();
}

void Processor_FishPond::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (!m_hasFrame)
    {
        return;
    }

    m_sprite.setPosition(m_projector.getTransformedPosition());
    const float scale = m_projector.getTransformedScale();
    m_sprite.setScale(scale, scale);
    if (m_shaderLoaded)
    {
        static sf::Clock time;
        const sf::Vector2u textureSize = m_texture.getSize();
        m_shader.setUniform("texelSize", sf::Glsl::Vec2(
            1.0f / textureSize.x,
            1.0f / textureSize.y));
        m_shader.setUniform("u_time", time.getElapsedTime().asSeconds());
        window.draw(m_sprite, &m_shader);
    }
    else
    {
        window.draw(m_sprite);
    }
    renderFish(window);
}

void Processor_FishPond::processEvent(
    const sf::Event & event,
    const sf::Vector2f & mouse)
{
    const bool draggingProjection = m_projector.processEvent(event, mouse);
    if (event.type != sf::Event::MouseButtonPressed
        || event.mouseButton.button != sf::Mouse::Left
        || draggingProjection || ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    cv::Point2f terrainPosition;
    if (mapMouseToTerrain(mouse, terrainPosition) && addFish(terrainPosition))
    {
        m_targetFishCount = std::min(MaximumFishCount, (int)m_fish.size());
    }
}

void Processor_FishPond::save(Save & save) const
{
    m_projector.save(save);
}

void Processor_FishPond::load(const Save & save)
{
    m_projector.load(save);
}

void Processor_FishPond::processTopography(const IntermediateData & data)
{
    PROFILE_FUNCTION();

    if (data.topography.empty() || data.topography.type() != CV_32F)
    {
        m_hasFrame = false;
        return;
    }

    if (m_topographySize.width > 0 && m_topographySize.height > 0
        && m_topographySize != data.topography.size())
    {
        const float xScale = (float)data.topography.cols / m_topographySize.width;
        const float yScale = (float)data.topography.rows / m_topographySize.height;
        for (Fish & fish : m_fish)
        {
            fish.position.x *= xScale;
            fish.position.y *= yScale;
            fish.velocity.x *= xScale;
            fish.velocity.y *= yScale;
        }
    }

    m_topography = data.topography;
    m_topographySize = data.topography.size();
    updateFish(data.deltaTime);

    m_projector.project(m_topography, m_projectedTopography);
    if (m_projectedTopography.empty())
    {
        m_hasFrame = false;
        return;
    }

    m_image = Tools::matToSfImage(m_projectedTopography);
    m_texture.loadFromImage(m_image);
    m_texture.setSmooth(true);
    m_sprite.setTexture(m_texture, true);
    m_hasFrame = true;
}
