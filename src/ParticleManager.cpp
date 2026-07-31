#include "ParticleManager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
    constexpr double Pi = 3.14159265358979323846;
    constexpr double TerrainTraversalSpeed = 0.60;
    constexpr double MinimumParticleSpeedMultiplier = 0.75;
    constexpr double MaximumParticleSpeedMultiplier = 1.25;
    constexpr size_t MaximumActiveParticles = 250000;

    double vectorLength(const sf::Vector2<double> & value)
    {
        return std::sqrt(value.x * value.x + value.y * value.y);
    }

    sf::Vector2<double> normalized(
        const sf::Vector2<double> & value,
        const sf::Vector2<double> & fallback = { 1.0, 0.0 })
    {
        const double length = vectorLength(value);
        return length > 0.000001
            ? sf::Vector2<double>(value.x / length, value.y / length)
            : fallback;
    }
}

void ParticleManager::update(const cv::Mat & data, float deltaTime)
{
    const int pixelWidth = data.cols;
    const int pixelHeight = data.rows;
    if (pixelWidth <= 0 || pixelHeight <= 0)
    {
        return;
    }

    AlgorithmParameters & selectedParameters = parameters[(size_t)Algorithm::Steering];
    const double configuredMinimum = std::clamp(
        (double)selectedParameters.minimumWindHeight,
        0.0,
        1.0);
    const double configuredMaximum = std::clamp(
        (double)selectedParameters.maximumWindHeight,
        configuredMinimum,
        1.0);
    const auto randomUnit = []()
    {
        return ((double)rand() + 0.5) / ((double)RAND_MAX + 1.0);
    };
    const auto terrainHeightAt = [&](double x, double y)
    {
        if (x < 0.0 || y < 0.0 || x >= pixelWidth || y >= pixelHeight)
        {
            return 1.0;
        }
        const int terrainX = std::clamp((int)std::round(x), 0, pixelWidth - 1);
        const int terrainY = std::clamp((int)std::round(y), 0, pixelHeight - 1);
        const float height = data.at<float>(terrainY, terrainX);
        return std::isfinite(height)
            ? std::clamp((double)height, 0.0, 1.0)
            : 1.0;
    };
    const auto placeParticle = [&](Particle & particle)
    {
        particle.pos.x = 0.0;
        particle.pos.y = rand() % pixelHeight;
        particle.height = configuredMinimum
            + (configuredMaximum - configuredMinimum) * randomUnit();
        particle.speedMultiplier = MinimumParticleSpeedMultiplier
            + (MaximumParticleSpeedMultiplier - MinimumParticleSpeedMultiplier) * randomUnit();
        particle.direction = { 1.0, 0.0 };
        particle.avoidanceSide = 0;
        particle.traversingTerrain = terrainHeightAt(
            particle.pos.x,
            particle.pos.y) > particle.height;
        particle.encounteredTraversalTerrain = particle.traversingTerrain;
    };

    if (m_framesUntilReset > 0)
    {
        m_framesUntilReset--;
        if (m_framesUntilReset == 0)
        {
            m_particles.clear();
            m_spawnAccumulator = 0.0;
        }
    }

    const float dt = std::clamp(deltaTime, 0.0f, 0.05f);
    m_spawnAccumulator += std::max(selectedParameters.spawnRate, 0.0f) * dt;
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
    for (int i = 0; i < particlesToSpawn; i++)
    {
        m_particles.emplace_back(0.0, 0.0);
        placeParticle(m_particles.back());
    }

    const double speed = std::max(0.0f, selectedParameters.particleSpeed);
    const double movement = speed * dt;
    const double lookAheadDistance = std::max({
        4.0,
        (double)selectedParameters.steeringDistance,
        movement * MaximumParticleSpeedMultiplier * 1.5 });
    const int lookAheadSamples = std::clamp(
        (int)std::ceil(lookAheadDistance / 3.0),
        8,
        48);
    const auto clearanceDistance = [&](const Particle & particle, const sf::Vector2<double> & direction)
    {
        for (int sample = 1; sample <= lookAheadSamples; sample++)
        {
            const double distance = lookAheadDistance * sample / lookAheadSamples;
            const double x = particle.pos.x + direction.x * distance;
            const double y = particle.pos.y + direction.y * distance;
            if (x >= pixelWidth - 1)
            {
                return lookAheadDistance;
            }
            if (x < 0.0 || y < 0.0 || y >= pixelHeight
                || terrainHeightAt(x, y) > particle.height)
            {
                return lookAheadDistance * (sample - 1) / lookAheadSamples;
            }
        }
        return lookAheadDistance;
    };
    const auto positionIsOpen = [&](const Particle & particle, const sf::Vector2<double> & position)
    {
        return position.x >= pixelWidth - 1
            || (position.x >= 0.0 && position.y >= 0.0 && position.y < pixelHeight
                && terrainHeightAt(position.x, position.y) <= particle.height);
    };

    static const std::array<double, 12> AvoidanceAngles = {
        -15.0, 15.0, -30.0, 30.0, -45.0, 45.0,
        -60.0, 60.0, -75.0, 75.0, -90.0, 90.0 };
    const sf::Vector2<double> right(1.0, 0.0);
    const auto terrainBurden = [&](const Particle & particle, const sf::Vector2<double> & direction)
    {
        double totalExcess = 0.0;
        double maximumExcess = 0.0;
        int samples = 0;
        for (int sample = 1; sample <= lookAheadSamples; sample++)
        {
            const double distance = lookAheadDistance * sample / lookAheadSamples;
            const double x = particle.pos.x + direction.x * distance;
            const double y = particle.pos.y + direction.y * distance;
            if (x >= pixelWidth - 1)
            {
                break;
            }
            if (x < 0.0 || y < 0.0 || y >= pixelHeight)
            {
                return 10.0;
            }

            const double excess = std::max(terrainHeightAt(x, y) - particle.height, 0.0);
            totalExcess += excess;
            maximumExcess = std::max(maximumExcess, excess);
            samples++;
        }
        return (samples > 0 ? totalExcess / samples : 0.0) + maximumExcess * 0.5;
    };
    const auto traversalDirection = [&](Particle & particle)
    {
        if (particle.avoidanceSide == 0)
        {
            particle.avoidanceSide = rand() % 2 == 0 ? -1 : 1;
        }

        sf::Vector2<double> bestDirection = right;
        double bestScore = -terrainBurden(particle, right) * 5.0
            + right.x * 0.65
            + particle.direction.x * 0.25;
        for (double angleDegrees : AvoidanceAngles)
        {
            const double angle = angleDegrees * Pi / 180.0;
            const sf::Vector2<double> candidate(std::cos(angle), std::sin(angle));
            const int candidateSide = candidate.y < 0.0 ? -1 : 1;
            const double continuity = candidate.x * particle.direction.x
                + candidate.y * particle.direction.y;
            const double score = -terrainBurden(particle, candidate) * 5.0
                + candidate.x * 0.65
                + continuity * 0.25
                + (candidateSide == particle.avoidanceSide ? 0.02 : 0.0);
            if (score > bestScore)
            {
                bestScore = score;
                bestDirection = candidate;
            }
        }
        if (std::abs(bestDirection.y) > 0.000001)
        {
            particle.avoidanceSide = bestDirection.y < 0.0 ? -1 : 1;
        }
        return bestDirection;
    };

    for (Particle & particle : m_particles)
    {
        const double particleMovement = movement * particle.speedMultiplier;
        particle.pos.x = std::clamp(particle.pos.x, 0.0, (double)(pixelWidth - 1));
        particle.pos.y = std::clamp(particle.pos.y, 0.0, (double)(pixelHeight - 1));

        const double currentTerrainHeight = terrainHeightAt(particle.pos.x, particle.pos.y);
        if (particle.traversingTerrain && currentTerrainHeight > particle.height)
        {
            particle.encounteredTraversalTerrain = true;
        }
        if (!particle.traversingTerrain && currentTerrainHeight > particle.height)
        {
            particle.traversingTerrain = true;
            particle.encounteredTraversalTerrain = true;
            particle.direction = right;
            particle.trail.clear();
        }

        if (particle.traversingTerrain
            && particle.encounteredTraversalTerrain
            && currentTerrainHeight <= particle.height
            && clearanceDistance(particle, right) >= lookAheadDistance * 0.999)
        {
            particle.traversingTerrain = false;
            particle.encounteredTraversalTerrain = false;
        }

        const double outletTraversalDistance = std::max(lookAheadDistance * 2.0, 32.0);
        const bool nearRightOutlet = particle.pos.x
            >= pixelWidth - 1 - outletTraversalDistance;
        bool outletTerrainAhead = false;
        if (!particle.traversingTerrain && nearRightOutlet)
        {
            const int firstX = std::clamp((int)std::ceil(particle.pos.x), 0, pixelWidth - 1);
            const int y = std::clamp((int)std::round(particle.pos.y), 0, pixelHeight - 1);
            for (int x = firstX; x < pixelWidth; x++)
            {
                if (terrainHeightAt(x, y) > particle.height)
                {
                    outletTerrainAhead = true;
                    break;
                }
            }
        }

        if (particle.traversingTerrain || outletTerrainAhead)
        {
            particle.traversingTerrain = true;
            const sf::Vector2<double> desiredDirection = nearRightOutlet
                ? right
                : traversalDirection(particle);
            const double steeringAmount = std::clamp(6.0 * dt, 0.0, 1.0);
            particle.direction = normalized({
                particle.direction.x + (desiredDirection.x - particle.direction.x) * steeringAmount,
                particle.direction.y + (desiredDirection.y - particle.direction.y) * steeringAmount });
            particle.pos.x += particle.direction.x * particleMovement * TerrainTraversalSpeed;
            particle.pos.y += particle.direction.y * particleMovement * TerrainTraversalSpeed;
        }
        else
        {
            sf::Vector2<double> desiredDirection = right;
            const double rightClearance = clearanceDistance(particle, right);
            const bool avoidingTerrain = rightClearance < lookAheadDistance * 0.999;
            if (avoidingTerrain)
            {
                if (particle.avoidanceSide == 0)
                {
                    particle.avoidanceSide = rand() % 2 == 0 ? -1 : 1;
                }

                double bestScore = -std::numeric_limits<double>::infinity();
                for (double angleDegrees : AvoidanceAngles)
                {
                    const double angle = angleDegrees * Pi / 180.0;
                    const sf::Vector2<double> candidate(std::cos(angle), std::sin(angle));
                    const double clearance = clearanceDistance(particle, candidate);
                    const int candidateSide = candidate.y < 0.0 ? -1 : 1;
                    const double continuity = candidate.x * particle.direction.x
                        + candidate.y * particle.direction.y;
                    const double score = clearance / lookAheadDistance * 2.0
                        + candidate.x * 0.40
                        + continuity * 0.35
                        + (candidateSide == particle.avoidanceSide ? 0.22 : 0.0);
                    if (score > bestScore)
                    {
                        bestScore = score;
                        desiredDirection = candidate;
                    }
                }
                particle.avoidanceSide = desiredDirection.y < 0.0 ? -1 : 1;
            }
            else
            {
                particle.avoidanceSide = 0;
            }

            const double steeringRate = avoidingTerrain ? 7.0 : 2.8;
            const double steeringAmount = std::clamp(steeringRate * dt, 0.0, 1.0);
            particle.direction = normalized({
                particle.direction.x + (desiredDirection.x - particle.direction.x) * steeringAmount,
                particle.direction.y + (desiredDirection.y - particle.direction.y) * steeringAmount });

            sf::Vector2<double> nextPosition(
                particle.pos.x + particle.direction.x * particleMovement,
                particle.pos.y + particle.direction.y * particleMovement);
            if (!positionIsOpen(particle, nextPosition))
            {
                particle.direction = normalized(desiredDirection);
                nextPosition = {
                    particle.pos.x + particle.direction.x * particleMovement,
                    particle.pos.y + particle.direction.y * particleMovement };
            }
            if (positionIsOpen(particle, nextPosition))
            {
                particle.pos = nextPosition;
            }
            else
            {
                particle.traversingTerrain = true;
                particle.encounteredTraversalTerrain = false;
                particle.direction = traversalDirection(particle);
                particle.pos.x += particle.direction.x * particleMovement * TerrainTraversalSpeed;
                particle.pos.y += particle.direction.y * particleMovement * TerrainTraversalSpeed;
            }
        }

        const bool reachedRightEdge = particle.pos.x >= pixelWidth - 1;
        const bool leftCanvas = particle.pos.x < 0.0
            || particle.pos.y < 0.0 || particle.pos.y >= pixelHeight;
        if (reachedRightEdge || leftCanvas)
        {
            continue;
        }

        particle.trail.push_back(particle.pos);
        while ((int)particle.trail.size() > selectedParameters.trailLength)
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
                return particle.pos.x >= pixelWidth - 1
                    || particle.pos.x < 0.0
                    || particle.pos.y < 0.0
                    || particle.pos.y >= pixelHeight;
            }),
        m_particles.end());
}
