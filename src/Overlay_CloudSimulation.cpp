#include "Overlay_CloudSimulation.h"

#include "Profiler.hpp"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;

    cv::Point2f normalized(const cv::Point2f & value)
    {
        const float length = std::sqrt(value.dot(value));
        return length > 0.0001f
            ? value * (1.0f / length)
            : cv::Point2f(1.0f, 0.0f);
    }
}

void Overlay_CloudSimulation::resetClouds()
{
    m_clouds.clear();
}

float Overlay_CloudSimulation::terrainHeight(
    const cv::Mat & terrain,
    const cv::Point2f & position) const
{
    if (terrain.empty() || terrain.type() != CV_32F
        || position.x < 0.0f || position.y < 0.0f
        || position.x >= terrain.cols || position.y >= terrain.rows)
    {
        return std::numeric_limits<float>::infinity();
    }

    const int x = std::clamp((int)std::round(position.x), 0, terrain.cols - 1);
    const int y = std::clamp((int)std::round(position.y), 0, terrain.rows - 1);
    const float height = terrain.at<float>(y, x);
    return std::isfinite(height) && height > 0.001f && height < 0.999f
        ? height
        : std::numeric_limits<float>::infinity();
}

bool Overlay_CloudSimulation::isOpenAir(
    const cv::Mat & terrain,
    const cv::Point2f & position) const
{
    return terrainHeight(terrain, position) < m_cloudHeight;
}

bool Overlay_CloudSimulation::findSpawnPosition(
    const cv::Mat & terrain,
    cv::Point2f & position)
{
    if (terrain.empty())
    {
        return false;
    }

    std::uniform_real_distribution<float> xDistribution(0.0f, std::max(0.0f, terrain.cols - 0.001f));
    std::uniform_real_distribution<float> yDistribution(0.0f, std::max(0.0f, terrain.rows - 0.001f));
    for (int attempt = 0; attempt < 512; attempt++)
    {
        const cv::Point2f candidate(xDistribution(m_random), yDistribution(m_random));
        if (isOpenAir(terrain, candidate))
        {
            position = candidate;
            return true;
        }
    }

    for (int y = 0; y < terrain.rows; y++)
    {
        for (int x = 0; x < terrain.cols; x++)
        {
            const cv::Point2f candidate((float)x, (float)y);
            if (isOpenAir(terrain, candidate))
            {
                position = candidate;
                return true;
            }
        }
    }
    return false;
}

float Overlay_CloudSimulation::pathScore(
    const cv::Mat & terrain,
    const cv::Point2f & position,
    const cv::Point2f & direction,
    float distance) const
{
    float minimumClearance = std::numeric_limits<float>::infinity();
    const cv::Point2f perpendicular(-direction.y, direction.x);
    const float halfWidth = std::max(
        1.0f,
        std::min(terrain.cols, terrain.rows) * 0.012f * m_cloudSize);
    for (int sample = 1; sample <= 6; sample++)
    {
        const cv::Point2f center = position + direction * (distance * sample / 6.0f);
        for (float offset : { -halfWidth, 0.0f, halfWidth })
        {
            cv::Point2f point = center + perpendicular * offset;
            if (point.y < 0.0f || point.y >= terrain.rows)
            {
                return -std::numeric_limits<float>::infinity();
            }

            point.x = std::fmod(point.x, (float)terrain.cols);
            if (point.x < 0.0f) { point.x += terrain.cols; }
            const float height = terrainHeight(terrain, point);
            if (!std::isfinite(height) || height >= m_cloudHeight)
            {
                return -std::numeric_limits<float>::infinity();
            }
            minimumClearance = std::min(minimumClearance, m_cloudHeight - height);
        }
    }
    return minimumClearance;
}

cv::Point2f Overlay_CloudSimulation::chooseDirection(
    const cv::Mat & terrain,
    const Cloud & cloud,
    float lookAhead) const
{
    constexpr std::array<float, 7> angles = { 0.0f, 15.0f, 30.0f, 45.0f, 62.0f, 78.0f, 90.0f };
    cv::Point2f bestDirection = cloud.direction;
    float bestScore = -std::numeric_limits<float>::infinity();

    for (float angle : angles)
    {
        const int sideCount = angle == 0.0f ? 1 : 2;
        for (int sideIndex = 0; sideIndex < sideCount; sideIndex++)
        {
            const int side = sideIndex == 0 ? cloud.avoidanceSide : -cloud.avoidanceSide;
            const float radians = angle * side * Pi / 180.0f;
            const cv::Point2f candidate(std::cos(radians), std::sin(radians));
            const float clearance = pathScore(terrain, cloud.position, candidate, lookAhead);
            if (!std::isfinite(clearance))
            {
                continue;
            }

            const float continuity = candidate.dot(cloud.direction);
            const float preferredSide = candidate.y * cloud.avoidanceSide >= 0.0f ? 0.08f : 0.0f;
            const float score = candidate.x * 2.5f + continuity * 0.55f
                + clearance * 1.5f + preferredSide;
            if (score > bestScore)
            {
                bestScore = score;
                bestDirection = candidate;
            }
        }
    }

    if (!std::isfinite(bestScore))
    {
        bestDirection = { 0.0f, (float)cloud.avoidanceSide };
    }
    return bestDirection;
}

void Overlay_CloudSimulation::relocateAfterWrap(
    const cv::Mat & terrain,
    Cloud & cloud)
{
    if (isOpenAir(terrain, cloud.position))
    {
        return;
    }

    const int startingY = std::clamp((int)std::round(cloud.position.y), 0, terrain.rows - 1);
    for (int offset = 1; offset < terrain.rows; offset++)
    {
        const int firstY = startingY + offset * cloud.avoidanceSide;
        const int secondY = startingY - offset * cloud.avoidanceSide;
        if (firstY >= 0 && firstY < terrain.rows
            && isOpenAir(terrain, { cloud.position.x, (float)firstY }))
        {
            cloud.position.y = (float)firstY;
            return;
        }
        if (secondY >= 0 && secondY < terrain.rows
            && isOpenAir(terrain, { cloud.position.x, (float)secondY }))
        {
            cloud.position.y = (float)secondY;
            return;
        }
    }

    cv::Point2f position;
    if (findSpawnPosition(terrain, position))
    {
        cloud.position = position;
    }
}

void Overlay_CloudSimulation::updateClouds(const cv::Mat & terrain, float deltaTime)
{
    std::uniform_real_distribution<float> phaseDistribution(0.0f, Pi * 2.0f);
    std::uniform_real_distribution<float> sizeDistribution(0.78f, 1.22f);
    std::uniform_int_distribution<int> sideDistribution(0, 1);
    while ((int)m_clouds.size() < m_cloudCount)
    {
        cv::Point2f position;
        if (!findSpawnPosition(terrain, position))
        {
            break;
        }
        Cloud cloud;
        cloud.position = position;
        cloud.phase = phaseDistribution(m_random);
        cloud.size = sizeDistribution(m_random);
        cloud.avoidanceSide = sideDistribution(m_random) == 0 ? -1 : 1;
        m_clouds.push_back(cloud);
    }
    if ((int)m_clouds.size() > m_cloudCount)
    {
        m_clouds.resize(m_cloudCount);
    }

    const float dt = std::clamp(deltaTime, 0.0f, 0.05f);
    const float speed = std::min(terrain.cols, terrain.rows) * 0.045f * m_speedMultiplier;
    const float lookAhead = std::max(5.0f, speed * 1.4f);
    const float steeringAmount = 1.0f - std::exp(-4.5f * dt);
    bool spawnUnavailable = false;

    for (Cloud & cloud : m_clouds)
    {
        if (!isOpenAir(terrain, cloud.position))
        {
            cv::Point2f position;
            if (!spawnUnavailable && findSpawnPosition(terrain, position))
            {
                cloud.position = position;
                cloud.direction = { 1.0f, 0.0f };
            }
            else
            {
                spawnUnavailable = true;
                cloud.position = { -1.0f, -1.0f };
            }
            continue;
        }

        const cv::Point2f desired = chooseDirection(terrain, cloud, lookAhead);
        cloud.direction = normalized(
            cloud.direction * (1.0f - steeringAmount)
            + desired * steeringAmount);

        cv::Point2f next = cloud.position + cloud.direction * (speed * dt);
        bool wrapped = false;
        if (next.x >= terrain.cols)
        {
            next.x = std::fmod(next.x, (float)terrain.cols);
            wrapped = true;
        }
        if (next.y < 0.0f || next.y >= terrain.rows)
        {
            cloud.avoidanceSide *= -1;
            cloud.direction.y *= -1.0f;
            next.y = std::clamp(next.y, 0.0f, terrain.rows - 0.001f);
        }

        if (wrapped)
        {
            cloud.position = next;
            relocateAfterWrap(terrain, cloud);
            cloud.direction = { 1.0f, 0.0f };
        }
        else if (isOpenAir(terrain, next))
        {
            cloud.position = next;
        }
        else
        {
            cloud.avoidanceSide *= -1;
        }
        cloud.phase = std::fmod(cloud.phase + dt * 0.35f, Pi * 2.0f);
    }
}

void Overlay_CloudSimulation::renderClouds(sf::RenderWindow & window)
{
    if (!m_processor || m_clouds.empty())
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
    points.reserve(m_clouds.size());
    for (const Cloud & cloud : m_clouds)
    {
        points.push_back(cloud.position);
    }
    cv::perspectiveTransform(points, points, projection);

    const sf::Vector2f origin = projector.getTransformedPosition();
    for (size_t i = 0; i < m_clouds.size(); i++)
    {
        if (!std::isfinite(points[i].x) || !std::isfinite(points[i].y))
        {
            continue;
        }

        const Cloud & cloud = m_clouds[i];
        const float groundHeight = terrainHeight(m_topography, cloud.position);
        if (!std::isfinite(groundHeight) || groundHeight >= m_cloudHeight)
        {
            continue;
        }
        const float clearance = std::max(m_cloudHeight - groundHeight, 0.0f);
        const float radius = 34.0f * cloud.size * m_cloudSize;
        const sf::Vector2f position(
            origin.x + points[i].x * scale,
            origin.y + points[i].y * scale + std::sin(cloud.phase) * 2.0f);

        sf::CircleShape shadow(radius * 0.90f, 32);
        shadow.setOrigin(shadow.getRadius(), shadow.getRadius());
        shadow.setPosition(position.x + radius * 0.25f, position.y + radius * 0.30f);
        shadow.setScale(1.35f, 0.68f);
        shadow.setFillColor(sf::Color(
            20,
            25,
            32,
            (sf::Uint8)(20 + 35 * std::clamp(1.0f - clearance, 0.0f, 1.0f))));
        window.draw(shadow);

        sf::CircleShape body(radius, 36);
        body.setOrigin(radius, radius);
        body.setPosition(position);
        body.setScale(1.42f, 0.70f);
        body.setFillColor(sf::Color(232, 238, 243, 165));
        window.draw(body);

        sf::CircleShape lobe(radius * 0.68f, 30);
        lobe.setOrigin(lobe.getRadius(), lobe.getRadius());
        lobe.setFillColor(sf::Color(248, 250, 252, 188));
        lobe.setPosition(position.x - radius * 0.58f, position.y - radius * 0.12f);
        window.draw(lobe);
        lobe.setPosition(position.x + radius * 0.52f, position.y - radius * 0.10f);
        lobe.setScale(0.84f, 0.84f);
        window.draw(lobe);
    }
}

void Overlay_CloudSimulation::initOverlay()
{
    resetClouds();
}

void Overlay_CloudSimulation::imguiOverlay()
{
    PROFILE_FUNCTION();

    ImGui::Text("Clouds: %d", (int)m_clouds.size());
    ImGui::SliderFloat("Cloud Height", &m_cloudHeight, 0.05f, 0.98f);
    ImGui::SliderInt("Cloud Count", &m_cloudCount, 1, 24);
    ImGui::SliderFloat("Cloud Speed", &m_speedMultiplier, 0.1f, 3.0f, "%.1fx");
    ImGui::SliderFloat("Cloud Size", &m_cloudSize, 0.5f, 2.0f);
    ImGui::TextUnformatted("Terrain at or above cloud height is avoided.");
    if (ImGui::Button("Reset Clouds"))
    {
        resetClouds();
    }
}

void Overlay_CloudSimulation::processTopographyOverlay(
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
        for (Cloud & cloud : m_clouds)
        {
            cloud.position.x *= xScale;
            cloud.position.y *= yScale;
        }
    }

    m_topography = data.topography;
    m_topographySize = data.topography.size();
    updateClouds(m_topography, data.deltaTime);
}

void Overlay_CloudSimulation::renderOverlay(
    sf::RenderWindow & window,
    TopographyProcessor & processor)
{
    m_processor = &processor;
    renderClouds(window);
}

void Overlay_CloudSimulation::processOverlayEvent(
    const sf::Event &,
    const sf::Vector2f &,
    TopographyProcessor &)
{
}

void Overlay_CloudSimulation::saveOverlay(Save &) const
{
}

void Overlay_CloudSimulation::loadOverlay(const Save &)
{
}
