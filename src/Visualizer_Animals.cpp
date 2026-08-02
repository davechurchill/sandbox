#include "Visualizer_Animals.h"
#include "Profiler.hpp"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;
}

void Visualizer_Animals::resetAnimals()
{
    m_sheep.clear();
    m_defaultSheepCreated = false;
    m_wolfCreated = false;
}

void Visualizer_Animals::randomizeDirection(Sheep & sheep)
{
    std::uniform_real_distribution<float> angleDistribution(0.0f, Pi * 2.0f);
    std::uniform_real_distribution<float> timerDistribution(1.5f, 4.5f);
    const float angle = angleDistribution(m_random);
    sheep.direction = { std::cos(angle), std::sin(angle) };
    sheep.desiredDirection = sheep.direction;
    sheep.directionTimer = timerDistribution(m_random);
    sheep.avoidingObstacle = false;
}

bool Visualizer_Animals::isValidTerrainPosition(const cv::Mat & terrain, const cv::Point2f & position) const
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
    return std::isfinite(height) && height > 0.001f && height < 0.999f;
}

bool Visualizer_Animals::spawnSheep(const cv::Point2f & position)
{
    if (!isValidTerrainPosition(m_topography, position))
    {
        return false;
    }

    Sheep sheep;
    sheep.position = position;
    std::uniform_real_distribution<float> phaseDistribution(0.0f, Pi * 2.0f);
    sheep.animationPhase = phaseDistribution(m_random);
    randomizeDirection(sheep);
    m_sheep.push_back(sheep);
    return true;
}

bool Visualizer_Animals::spawnRandomSheep(const cv::Mat & terrain)
{
    if (terrain.empty())
    {
        return false;
    }

    std::uniform_int_distribution<int> xDistribution(0, terrain.cols - 1);
    std::uniform_int_distribution<int> yDistribution(0, terrain.rows - 1);
    for (int attempt = 0; attempt < 1024; attempt++)
    {
        const cv::Point2f position((float)xDistribution(m_random), (float)yDistribution(m_random));
        if (spawnSheep(position))
        {
            return true;
        }
    }

    for (int y = 0; y < terrain.rows; y++)
    {
        for (int x = 0; x < terrain.cols; x++)
        {
            if (isValidTerrainPosition(terrain, { (float)x, (float)y }))
            {
                return spawnSheep({ (float)x, (float)y });
            }
        }
    }

    return false;
}

bool Visualizer_Animals::spawnRandomWolf(const cv::Mat & terrain)
{
    if (terrain.empty())
    {
        return false;
    }

    const float minimumDistance = std::min(terrain.cols, terrain.rows) * 0.18f;
    const float minimumDistanceSquared = minimumDistance * minimumDistance;
    const float maximumDistanceSquared = (float)(terrain.cols * terrain.cols + terrain.rows * terrain.rows);
    float bestDistanceSquared = -1.0f;
    cv::Point2f bestPosition;

    const auto nearestSheepDistanceSquared = [&](const cv::Point2f & position)
    {
        float nearestDistanceSquared = maximumDistanceSquared;
        for (const Sheep & sheep : m_sheep)
        {
            const float dx = sheep.position.x - position.x;
            const float dy = sheep.position.y - position.y;
            nearestDistanceSquared = std::min(nearestDistanceSquared, dx * dx + dy * dy);
        }
        return nearestDistanceSquared;
    };

    const auto createWolf = [&](const cv::Point2f & position)
    {
        m_wolf = Wolf{};
        m_wolf.position = position;
        std::uniform_real_distribution<float> angleDistribution(0.0f, Pi * 2.0f);
        const float angle = angleDistribution(m_random);
        m_wolf.direction = { std::cos(angle), std::sin(angle) };
        m_wolf.desiredDirection = m_wolf.direction;
        m_wolfCreated = true;
    };

    std::uniform_int_distribution<int> xDistribution(0, terrain.cols - 1);
    std::uniform_int_distribution<int> yDistribution(0, terrain.rows - 1);
    for (int attempt = 0; attempt < 1024; attempt++)
    {
        const cv::Point2f position((float)xDistribution(m_random), (float)yDistribution(m_random));
        if (!isValidTerrainPosition(terrain, position))
        {
            continue;
        }

        const float distanceSquared = nearestSheepDistanceSquared(position);
        if (distanceSquared > bestDistanceSquared)
        {
            bestDistanceSquared = distanceSquared;
            bestPosition = position;
        }
        if (distanceSquared >= minimumDistanceSquared)
        {
            createWolf(position);
            return true;
        }
    }

    if (bestDistanceSquared >= 0.0f)
    {
        createWolf(bestPosition);
        return true;
    }

    for (int y = 0; y < terrain.rows; y++)
    {
        for (int x = 0; x < terrain.cols; x++)
        {
            const cv::Point2f position((float)x, (float)y);
            if (isValidTerrainPosition(terrain, position))
            {
                createWolf(position);
                return true;
            }
        }
    }

    return false;
}

void Visualizer_Animals::updateSheep(const cv::Mat & terrain, float deltaTime)
{
    if (!m_defaultSheepCreated)
    {
        while (m_sheep.size() < 2 && spawnRandomSheep(terrain))
        {
        }
        m_defaultSheepCreated = m_sheep.size() >= 2;
    }

    const float dt = std::clamp(deltaTime, 0.0f, 0.1f);
    const float speed = std::min(terrain.cols, terrain.rows) * 0.03f * m_sheepSpeed;
    std::uniform_real_distribution<float> turnDistribution(-Pi * 0.6f, Pi * 0.6f);

    for (Sheep & sheep : m_sheep)
    {
        if (!isValidTerrainPosition(terrain, sheep.position))
        {
            bool relocated = false;
            for (int attempt = 0; attempt < 256 && !relocated; attempt++)
            {
                std::uniform_int_distribution<int> xDistribution(0, terrain.cols - 1);
                std::uniform_int_distribution<int> yDistribution(0, terrain.rows - 1);
                const cv::Point2f position((float)xDistribution(m_random), (float)yDistribution(m_random));
                if (isValidTerrainPosition(terrain, position))
                {
                    sheep.position = position;
                    randomizeDirection(sheep);
                    relocated = true;
                }
            }
            if (!relocated)
            {
                continue;
            }
        }

        const int currentX = std::clamp((int)std::round(sheep.position.x), 0, terrain.cols - 1);
        const int currentY = std::clamp((int)std::round(sheep.position.y), 0, terrain.rows - 1);
        const float currentHeight = terrain.at<float>(currentY, currentX);
        const float lookAheadDistance = std::max(3.0f, speed * 0.25f);

        const auto directionIsClear = [&](const cv::Point2f & direction)
        {
            for (int sample = 1; sample <= 3; sample++)
            {
                const float distance = lookAheadDistance * sample / 3.0f;
                const cv::Point2f lookAhead(
                    sheep.position.x + direction.x * distance,
                    sheep.position.y + direction.y * distance);
                if (!isValidTerrainPosition(terrain, lookAhead))
                {
                    return false;
                }

                const int lookAheadX = std::clamp((int)std::round(lookAhead.x), 0, terrain.cols - 1);
                const int lookAheadY = std::clamp((int)std::round(lookAhead.y), 0, terrain.rows - 1);
                const float heightDifference = std::abs(terrain.at<float>(lookAheadY, lookAheadX) - currentHeight);
                if (heightDifference / distance > 0.025f)
                {
                    return false;
                }
            }

            return true;
        };

        const auto beginAvoidingObstacle = [&]()
        {
            const float angle = std::atan2(sheep.direction.y, sheep.direction.x);
            const float leftAngle = angle - Pi * 0.5f;
            const float rightAngle = angle + Pi * 0.5f;
            const cv::Point2f leftDirection(std::cos(leftAngle), std::sin(leftAngle));
            const cv::Point2f rightDirection(std::cos(rightAngle), std::sin(rightAngle));
            const bool leftIsClear = directionIsClear(leftDirection);
            const bool rightIsClear = directionIsClear(rightDirection);

            if (leftIsClear && rightIsClear)
            {
                std::uniform_int_distribution<int> sideDistribution(0, 1);
                sheep.desiredDirection = sideDistribution(m_random) == 0 ? leftDirection : rightDirection;
            }
            else if (leftIsClear)
            {
                sheep.desiredDirection = leftDirection;
            }
            else if (rightIsClear)
            {
                sheep.desiredDirection = rightDirection;
            }
            else
            {
                sheep.desiredDirection = { -sheep.direction.x, -sheep.direction.y };
            }

            sheep.directionTimer = 1.5f;
            sheep.avoidingObstacle = true;
        };

        bool fleeingWolf = false;
        if (m_wolfCreated)
        {
            const float dx = sheep.position.x - m_wolf.position.x;
            const float dy = sheep.position.y - m_wolf.position.y;
            const float distanceSquared = dx * dx + dy * dy;
            const float threatRadius = std::min(terrain.cols, terrain.rows) * 0.14f;
            if (distanceSquared < threatRadius * threatRadius && distanceSquared > 0.0001f)
            {
                fleeingWolf = true;
                if (!sheep.avoidingObstacle)
                {
                    const float inverseDistance = 1.0f / std::sqrt(distanceSquared);
                    sheep.desiredDirection = { dx * inverseDistance, dy * inverseDistance };
                    sheep.directionTimer = 0.75f;
                }
            }
        }

        sheep.directionTimer -= dt;
        if (sheep.directionTimer <= 0.0f && !sheep.avoidingObstacle && !fleeingWolf)
        {
            const float currentAngle = std::atan2(sheep.direction.y, sheep.direction.x);
            const float angle = currentAngle + turnDistribution(m_random);
            sheep.desiredDirection = { std::cos(angle), std::sin(angle) };
            std::uniform_real_distribution<float> timerDistribution(1.5f, 4.5f);
            sheep.directionTimer = timerDistribution(m_random);
        }

        if (!directionIsClear(sheep.direction) && !sheep.avoidingObstacle)
        {
            beginAvoidingObstacle();
        }

        const float currentAngle = std::atan2(sheep.direction.y, sheep.direction.x);
        const float desiredAngle = std::atan2(sheep.desiredDirection.y, sheep.desiredDirection.x);
        const float angleDifference = std::atan2(
            std::sin(desiredAngle - currentAngle),
            std::cos(desiredAngle - currentAngle));
        const float maximumTurn = (fleeingWolf ? 2.8f : 1.8f) * dt;
        const float steeredAngle = currentAngle + std::clamp(angleDifference, -maximumTurn, maximumTurn);
        sheep.direction = { std::cos(steeredAngle), std::sin(steeredAngle) };

        const bool headingIsClear = directionIsClear(sheep.direction);
        const cv::Point2f slopeProbe(
            sheep.position.x + sheep.direction.x,
            sheep.position.y + sheep.direction.y);

        float uphillSpeed = 1.0f;
        if (isValidTerrainPosition(terrain, slopeProbe))
        {
            const int probeX = std::clamp((int)std::round(slopeProbe.x), 0, terrain.cols - 1);
            const int probeY = std::clamp((int)std::round(slopeProbe.y), 0, terrain.rows - 1);
            const float heightRise = terrain.at<float>(probeY, probeX) - currentHeight;
            if (heightRise > 0.0f)
            {
                uphillSpeed = std::clamp(1.0f - heightRise * 45.0f, 0.25f, 1.0f);
            }
        }

        const cv::Point2f proposed(
            sheep.position.x + sheep.direction.x * speed * uphillSpeed * dt,
            sheep.position.y + sheep.direction.y * speed * uphillSpeed * dt);

        bool canMove = headingIsClear && isValidTerrainPosition(terrain, proposed);
        if (canMove)
        {
            const int proposedX = std::clamp((int)std::round(proposed.x), 0, terrain.cols - 1);
            const int proposedY = std::clamp((int)std::round(proposed.y), 0, terrain.rows - 1);
            const float proposedHeight = terrain.at<float>(proposedY, proposedX);
            canMove = std::abs(proposedHeight - currentHeight) <= 0.035f;
        }

        if (canMove)
        {
            sheep.position = proposed;
            sheep.animationPhase += dt * uphillSpeed * (5.0f + m_sheepSpeed * 2.0f);
            if (sheep.avoidingObstacle)
            {
                sheep.avoidingObstacle = false;
                sheep.directionTimer = std::max(sheep.directionTimer, 1.0f);
            }
        }
        else if (!sheep.avoidingObstacle)
        {
            beginAvoidingObstacle();
        }
    }
}

void Visualizer_Animals::updateWolf(const cv::Mat & terrain, float deltaTime)
{
    if (!m_wolfCreated && !spawnRandomWolf(terrain))
    {
        return;
    }

    if (!isValidTerrainPosition(terrain, m_wolf.position))
    {
        m_wolfCreated = false;
        if (!spawnRandomWolf(terrain))
        {
            return;
        }
    }

    if (m_sheep.empty())
    {
        return;
    }

    const Sheep * nearestSheep = &m_sheep.front();
    float nearestDistanceSquared = std::numeric_limits<float>::max();
    for (const Sheep & sheep : m_sheep)
    {
        const float dx = sheep.position.x - m_wolf.position.x;
        const float dy = sheep.position.y - m_wolf.position.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < nearestDistanceSquared)
        {
            nearestDistanceSquared = distanceSquared;
            nearestSheep = &sheep;
        }
    }

    const float targetX = nearestSheep->position.x - m_wolf.position.x;
    const float targetY = nearestSheep->position.y - m_wolf.position.y;
    const float targetDistance = std::sqrt(std::max(nearestDistanceSquared, 0.0001f));
    const cv::Point2f targetDirection(targetX / targetDistance, targetY / targetDistance);

    const float dt = std::clamp(deltaTime, 0.0f, 0.1f);
    const float sheepSpeed = std::min(terrain.cols, terrain.rows) * 0.03f * m_sheepSpeed;
    const float wolfSpeed = sheepSpeed * 1.20f;
    const int currentX = std::clamp((int)std::round(m_wolf.position.x), 0, terrain.cols - 1);
    const int currentY = std::clamp((int)std::round(m_wolf.position.y), 0, terrain.rows - 1);
    const float currentHeight = terrain.at<float>(currentY, currentX);
    const float lookAheadDistance = std::max(3.0f, wolfSpeed * 0.25f);

    const auto directionIsClear = [&](const cv::Point2f & direction)
    {
        for (int sample = 1; sample <= 3; sample++)
        {
            const float distance = lookAheadDistance * sample / 3.0f;
            const cv::Point2f lookAhead(
                m_wolf.position.x + direction.x * distance,
                m_wolf.position.y + direction.y * distance);
            if (!isValidTerrainPosition(terrain, lookAhead))
            {
                return false;
            }

            const int lookAheadX = std::clamp((int)std::round(lookAhead.x), 0, terrain.cols - 1);
            const int lookAheadY = std::clamp((int)std::round(lookAhead.y), 0, terrain.rows - 1);
            const float heightDifference = std::abs(terrain.at<float>(lookAheadY, lookAheadX) - currentHeight);
            if (heightDifference / distance > 0.025f)
            {
                return false;
            }
        }

        return true;
    };

    m_wolf.avoidanceTimer = std::max(0.0f, m_wolf.avoidanceTimer - dt);
    if (directionIsClear(targetDirection))
    {
        m_wolf.desiredDirection = targetDirection;
        m_wolf.avoidanceTimer = 0.0f;
    }
    else if (m_wolf.avoidanceTimer <= 0.0f || !directionIsClear(m_wolf.desiredDirection))
    {
        const float targetAngle = std::atan2(targetDirection.y, targetDirection.x);
        std::uniform_int_distribution<int> sideDistribution(0, 1);
        const float side = sideDistribution(m_random) == 0 ? -1.0f : 1.0f;
        const float offsets[] = {
            side * Pi * 0.25f,
            -side * Pi * 0.25f,
            side * Pi * 0.5f,
            -side * Pi * 0.5f,
            side * Pi * 0.75f,
            -side * Pi * 0.75f,
            Pi
        };

        bool routeFound = false;
        for (float offset : offsets)
        {
            const float angle = targetAngle + offset;
            const cv::Point2f candidate(std::cos(angle), std::sin(angle));
            if (directionIsClear(candidate))
            {
                m_wolf.desiredDirection = candidate;
                routeFound = true;
                break;
            }
        }

        if (!routeFound)
        {
            m_wolf.desiredDirection = { -m_wolf.direction.x, -m_wolf.direction.y };
        }
        m_wolf.avoidanceTimer = 0.75f;
    }

    const float currentAngle = std::atan2(m_wolf.direction.y, m_wolf.direction.x);
    const float desiredAngle = std::atan2(m_wolf.desiredDirection.y, m_wolf.desiredDirection.x);
    const float angleDifference = std::atan2(
        std::sin(desiredAngle - currentAngle),
        std::cos(desiredAngle - currentAngle));
    const float maximumTurn = 2.2f * dt;
    const float steeredAngle = currentAngle + std::clamp(angleDifference, -maximumTurn, maximumTurn);
    m_wolf.direction = { std::cos(steeredAngle), std::sin(steeredAngle) };

    const cv::Point2f slopeProbe(
        m_wolf.position.x + m_wolf.direction.x,
        m_wolf.position.y + m_wolf.direction.y);
    float uphillSpeed = 1.0f;
    if (isValidTerrainPosition(terrain, slopeProbe))
    {
        const int probeX = std::clamp((int)std::round(slopeProbe.x), 0, terrain.cols - 1);
        const int probeY = std::clamp((int)std::round(slopeProbe.y), 0, terrain.rows - 1);
        const float heightRise = terrain.at<float>(probeY, probeX) - currentHeight;
        if (heightRise > 0.0f)
        {
            uphillSpeed = std::clamp(1.0f - heightRise * 45.0f, 0.25f, 1.0f);
        }
    }

    const cv::Point2f proposed(
        m_wolf.position.x + m_wolf.direction.x * wolfSpeed * uphillSpeed * dt,
        m_wolf.position.y + m_wolf.direction.y * wolfSpeed * uphillSpeed * dt);
    bool canMove = directionIsClear(m_wolf.direction) && isValidTerrainPosition(terrain, proposed);
    if (canMove)
    {
        const int proposedX = std::clamp((int)std::round(proposed.x), 0, terrain.cols - 1);
        const int proposedY = std::clamp((int)std::round(proposed.y), 0, terrain.rows - 1);
        const float proposedHeight = terrain.at<float>(proposedY, proposedX);
        canMove = std::abs(proposedHeight - currentHeight) <= 0.035f;
    }

    if (canMove)
    {
        m_wolf.position = proposed;
        m_wolf.animationPhase += dt * uphillSpeed * (6.0f + m_sheepSpeed * 2.0f);
    }
    else
    {
        m_wolf.avoidanceTimer = 0.0f;
    }

    const float eatRadius = std::max(2.0f, std::min(terrain.cols, terrain.rows) * 0.015f);
    const float eatRadiusSquared = eatRadius * eatRadius;
    m_sheep.erase(
        std::remove_if(
            m_sheep.begin(),
            m_sheep.end(),
            [&](const Sheep & sheep)
            {
                const float dx = sheep.position.x - m_wolf.position.x;
                const float dy = sheep.position.y - m_wolf.position.y;
                return dx * dx + dy * dy <= eatRadiusSquared;
            }),
        m_sheep.end());
}

bool Visualizer_Animals::mapMouseToTerrain(const sf::Vector2f & mouse, cv::Point2f & terrainPosition)
{
    if (m_topography.empty())
    {
        return false;
    }

    SandboxProjector & overlayProjector = projector();
    const float scale = overlayProjector.getTransformedScale();
    if (!std::isfinite(scale) || scale <= 0.0f)
    {
        return false;
    }

    const sf::Vector2f offset = mouse - overlayProjector.getTransformedPosition();
    std::vector<cv::Point2f> point = { { offset.x / scale, offset.y / scale } };
    const cv::Mat projection = overlayProjector.getProjectionMatrix();
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
    return isValidTerrainPosition(m_topography, terrainPosition);
}

void Visualizer_Animals::drawSheep(
    sf::RenderWindow & window,
    const sf::Vector2f & position,
    const sf::Vector2f & direction,
    const Sheep & sheep) const
{
    const float size = m_sheepSize;
    const float directionLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    const sf::Vector2f forward = directionLength > 0.001f
        ? direction / directionLength
        : sf::Vector2f(1.0f, 0.0f);
    const sf::Vector2f right(-forward.y, forward.x);
    const float angle = std::atan2(forward.y, forward.x) * 180.0f / Pi;
    const float sway = std::sin(sheep.animationPhase) * size * 0.035f;
    const sf::Vector2f bodyPosition = position + right * sway;

    sf::CircleShape shadow(size * 0.48f, 24);
    shadow.setOrigin({ size * 0.48f, size * 0.48f });
    shadow.setPosition(bodyPosition + sf::Vector2f(size * 0.08f, size * 0.10f));
    shadow.setScale({ 1.38f, 0.78f });
    shadow.setRotation(sf::degrees(angle));
    shadow.setFillColor(sf::Color(18, 32, 12, 85));
    window.draw(shadow);

    sf::CircleShape hoof(size * 0.10f, 12);
    hoof.setOrigin({ size * 0.10f, size * 0.10f });
    hoof.setFillColor(sf::Color(57, 51, 45));
    for (float along : { -0.30f, 0.28f })
    {
        hoof.setPosition(bodyPosition + forward * (size * along) + right * (size * 0.35f));
        window.draw(hoof);
        hoof.setPosition(bodyPosition + forward * (size * along) - right * (size * 0.35f));
        window.draw(hoof);
    }

    sf::CircleShape body(size * 0.5f, 32);
    body.setOrigin({ size * 0.5f, size * 0.5f });
    body.setPosition(bodyPosition);
    body.setScale({ 1.35f, 0.78f });
    body.setRotation(sf::degrees(angle));
    body.setFillColor(sf::Color(238, 235, 216));
    body.setOutlineColor(sf::Color(172, 170, 152));
    body.setOutlineThickness(std::max(1.0f, size * 0.06f));
    window.draw(body);

    sf::CircleShape wool(size * 0.22f, 20);
    wool.setOrigin({ size * 0.22f, size * 0.22f });
    wool.setFillColor(sf::Color(250, 248, 231));
    wool.setPosition(bodyPosition - forward * (size * 0.30f));
    window.draw(wool);
    wool.setPosition(bodyPosition + right * (size * 0.19f));
    window.draw(wool);
    wool.setPosition(bodyPosition - right * (size * 0.19f));
    window.draw(wool);

    const sf::Vector2f headPosition = bodyPosition + forward * (size * 0.67f);

    sf::CircleShape ear(size * 0.13f, 16);
    ear.setOrigin({ size * 0.13f, size * 0.13f });
    ear.setScale({ 1.0f, 0.45f });
    ear.setRotation(sf::degrees(angle + 90.0f));
    ear.setFillColor(sf::Color(64, 57, 51));
    ear.setPosition(headPosition + right * (size * 0.22f) - forward * (size * 0.06f));
    window.draw(ear);
    ear.setPosition(headPosition - right * (size * 0.22f) - forward * (size * 0.06f));
    window.draw(ear);

    sf::CircleShape head(size * 0.25f, 24);
    head.setOrigin({ size * 0.25f, size * 0.25f });
    head.setPosition(headPosition);
    head.setScale({ 1.10f, 0.78f });
    head.setRotation(sf::degrees(angle));
    head.setFillColor(sf::Color(73, 66, 58));
    window.draw(head);

    sf::CircleShape eye(std::max(0.8f, size * 0.04f), 12);
    eye.setOrigin({ eye.getRadius(), eye.getRadius() });
    eye.setFillColor(sf::Color::Black);
    eye.setPosition(headPosition + forward * (size * 0.10f) + right * (size * 0.09f));
    window.draw(eye);
    eye.setPosition(headPosition + forward * (size * 0.10f) - right * (size * 0.09f));
    window.draw(eye);
}

void Visualizer_Animals::renderSheep(sf::RenderWindow & window)
{
    if (m_sheep.empty())
    {
        return;
    }

    SandboxProjector & overlayProjector = projector();
    const cv::Mat projection = overlayProjector.getProjectionMatrix();
    const float scale = overlayProjector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    std::vector<cv::Point2f> projectedPoints;
    projectedPoints.reserve(m_sheep.size() * 2);
    for (const Sheep & sheep : m_sheep)
    {
        projectedPoints.push_back(sheep.position);
        projectedPoints.push_back({
            sheep.position.x + sheep.direction.x * 4.0f,
            sheep.position.y + sheep.direction.y * 4.0f });
    }
    cv::perspectiveTransform(projectedPoints, projectedPoints, projection);

    const sf::Vector2f origin = overlayProjector.getTransformedPosition();
    for (size_t index = 0; index < m_sheep.size(); index++)
    {
        const cv::Point2f & point = projectedPoints[index * 2];
        const cv::Point2f & ahead = projectedPoints[index * 2 + 1];
        if (!std::isfinite(point.x) || !std::isfinite(point.y)
            || !std::isfinite(ahead.x) || !std::isfinite(ahead.y))
        {
            continue;
        }

        drawSheep(
            window,
            { origin.x + point.x * scale, origin.y + point.y * scale },
            { ahead.x - point.x, ahead.y - point.y },
            m_sheep[index]);
    }
}

void Visualizer_Animals::drawWolf(
    sf::RenderWindow & window,
    const sf::Vector2f & position,
    const sf::Vector2f & direction) const
{
    const float size = m_sheepSize * 1.45f;
    const float directionLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    const sf::Vector2f forward = directionLength > 0.001f
        ? direction / directionLength
        : sf::Vector2f(1.0f, 0.0f);
    const sf::Vector2f right(-forward.y, forward.x);
    const float angle = std::atan2(forward.y, forward.x) * 180.0f / Pi;
    const float stride = std::sin(m_wolf.animationPhase) * size * 0.035f;
    const sf::Vector2f bodyPosition = position + right * stride;

    sf::CircleShape shadow(size * 0.48f, 24);
    shadow.setOrigin({ size * 0.48f, size * 0.48f });
    shadow.setPosition(bodyPosition + sf::Vector2f(size * 0.08f, size * 0.10f));
    shadow.setScale({ 1.45f, 0.67f });
    shadow.setRotation(sf::degrees(angle));
    shadow.setFillColor(sf::Color(12, 19, 10, 90));
    window.draw(shadow);

    sf::RectangleShape tail({ size * 0.65f, size * 0.13f });
    tail.setOrigin({ size * 0.65f, size * 0.065f });
    tail.setPosition(bodyPosition - forward * (size * 0.48f));
    tail.setRotation(sf::degrees(angle + std::sin(m_wolf.animationPhase * 0.55f) * 18.0f));
    tail.setFillColor(sf::Color(78, 82, 79));
    window.draw(tail);

    sf::CircleShape paw(size * 0.09f, 12);
    paw.setOrigin({ size * 0.09f, size * 0.09f });
    paw.setFillColor(sf::Color(43, 45, 43));
    for (float along : { -0.27f, 0.27f })
    {
        paw.setPosition(bodyPosition + forward * (size * along) + right * (size * 0.30f));
        window.draw(paw);
        paw.setPosition(bodyPosition + forward * (size * along) - right * (size * 0.30f));
        window.draw(paw);
    }

    sf::CircleShape body(size * 0.5f, 28);
    body.setOrigin({ size * 0.5f, size * 0.5f });
    body.setPosition(bodyPosition);
    body.setScale({ 1.38f, 0.64f });
    body.setRotation(sf::degrees(angle));
    body.setFillColor(sf::Color(91, 96, 92));
    body.setOutlineColor(sf::Color(48, 51, 49));
    body.setOutlineThickness(std::max(1.0f, size * 0.055f));
    window.draw(body);

    const sf::Vector2f headPosition = bodyPosition + forward * (size * 0.62f);
    sf::CircleShape head(size * 0.28f, 22);
    head.setOrigin({ size * 0.28f, size * 0.28f });
    head.setPosition(headPosition);
    head.setScale({ 1.05f, 0.76f });
    head.setRotation(sf::degrees(angle));
    head.setFillColor(sf::Color(72, 76, 73));
    window.draw(head);

    sf::ConvexShape ear(3);
    ear.setPoint(0, { 0.0f, 0.0f });
    ear.setPoint(1, { -size * 0.25f, -size * 0.10f });
    ear.setPoint(2, { -size * 0.13f, size * 0.10f });
    ear.setFillColor(sf::Color(47, 50, 48));
    ear.setPosition(headPosition + right * (size * 0.18f));
    ear.setRotation(sf::degrees(angle));
    window.draw(ear);
    ear.setScale({ 1.0f, -1.0f });
    ear.setPosition(headPosition - right * (size * 0.18f));
    window.draw(ear);

    sf::ConvexShape muzzle(3);
    muzzle.setPoint(0, { 0.0f, -size * 0.15f });
    muzzle.setPoint(1, { size * 0.36f, 0.0f });
    muzzle.setPoint(2, { 0.0f, size * 0.15f });
    muzzle.setPosition(headPosition + forward * (size * 0.10f));
    muzzle.setRotation(sf::degrees(angle));
    muzzle.setFillColor(sf::Color(58, 61, 59));
    window.draw(muzzle);

    sf::CircleShape eye(std::max(0.8f, size * 0.038f), 12);
    eye.setOrigin({ eye.getRadius(), eye.getRadius() });
    eye.setFillColor(sf::Color(224, 181, 54));
    eye.setPosition(headPosition + forward * (size * 0.07f) + right * (size * 0.10f));
    window.draw(eye);
    eye.setPosition(headPosition + forward * (size * 0.07f) - right * (size * 0.10f));
    window.draw(eye);
}

void Visualizer_Animals::renderWolf(sf::RenderWindow & window)
{
    if (!m_wolfCreated)
    {
        return;
    }

    SandboxProjector & overlayProjector = projector();
    const cv::Mat projection = overlayProjector.getProjectionMatrix();
    const float scale = overlayProjector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    std::vector<cv::Point2f> projectedPoints = {
        m_wolf.position,
        {
            m_wolf.position.x + m_wolf.direction.x * 4.0f,
            m_wolf.position.y + m_wolf.direction.y * 4.0f
        }
    };
    cv::perspectiveTransform(projectedPoints, projectedPoints, projection);

    const cv::Point2f & point = projectedPoints[0];
    const cv::Point2f & ahead = projectedPoints[1];
    if (!std::isfinite(point.x) || !std::isfinite(point.y)
        || !std::isfinite(ahead.x) || !std::isfinite(ahead.y))
    {
        return;
    }

    const sf::Vector2f origin = overlayProjector.getTransformedPosition();
    drawWolf(
        window,
        { origin.x + point.x * scale, origin.y + point.y * scale },
        { ahead.x - point.x, ahead.y - point.y });
}

void Visualizer_Animals::init()
{
    resetAnimals();
}

void Visualizer_Animals::imgui()
{
    PROFILE_FUNCTION();

    ImGui::Text("Sheep: %d", (int)m_sheep.size());
    ImGui::TextUnformatted(m_wolfCreated ? "Wolf: hunting" : "Wolf: waiting for terrain");
    ImGui::SliderFloat("Sheep Speed", &m_sheepSpeed, 0.1f, 3.0f);
    ImGui::SliderFloat("Sheep Size", &m_sheepSize, 5.0f, 24.0f);
    ImGui::TextUnformatted("Left mouse: add sheep");

    if (ImGui::Button("Reset Animals"))
    {
        resetAnimals();
    }
}

void Visualizer_Animals::process(
    const TerrainFrame & data)
{
    if (data.heightMap.empty() || data.heightMap.type() != CV_32F)
    {
        return;
    }

    if (m_topographySize.width > 0 && m_topographySize.height > 0
        && m_topographySize != data.heightMap.size())
    {
        const float xScale = (float)data.heightMap.cols / m_topographySize.width;
        const float yScale = (float)data.heightMap.rows / m_topographySize.height;
        for (Sheep & sheep : m_sheep)
        {
            sheep.position.x *= xScale;
            sheep.position.y *= yScale;
        }
        if (m_wolfCreated)
        {
            m_wolf.position.x *= xScale;
            m_wolf.position.y *= yScale;
        }
    }

    m_topography = data.heightMap;
    m_topographySize = data.heightMap.size();
    updateSheep(m_topography, data.deltaTime);
    updateWolf(m_topography, data.deltaTime);
}

void Visualizer_Animals::render(
    sf::RenderWindow & window)
{
    renderSheep(window);
    renderWolf(window);
}

void Visualizer_Animals::processEvent(
    const sf::Event & event,
    const sf::Vector2f & mouse)
{
    const bool draggingProjection = projector().processEvent(event, mouse);
    const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>();
    if (!mousePressed || mousePressed->button != sf::Mouse::Button::Left
        || draggingProjection || ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    cv::Point2f terrainPosition;
    if (mapMouseToTerrain(mouse, terrainPosition))
    {
        spawnSheep(terrainPosition);
    }
}

void Visualizer_Animals::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_Animals");
    settings["m_sheepSpeed"] = m_sheepSpeed;
    settings["m_sheepSize"] = m_sheepSize;
}

void Visualizer_Animals::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_Animals");
    Settings::read(settings, "m_sheepSpeed", m_sheepSpeed);
    Settings::read(settings, "m_sheepSize", m_sheepSize);
}
