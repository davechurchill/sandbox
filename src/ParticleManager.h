#pragma once

#include <opencv2/core.hpp>
#include <vector>

#include "SFML/System/Vector2.hpp"

struct Particle
{
    sf::Vector2<double> pos{ 0.0, 0.0 };
    sf::Vector2<double> direction{ 1.0, 0.0 };
    std::vector<sf::Vector2<double>> trail{};
    double height = 1.0;
    double speedMultiplier = 1.0;
    int avoidanceSide = 0;
    bool traversingTerrain = false;
    bool encounteredTraversalTerrain = false;

    Particle(double x, double y) : pos({ x, y })
    {
    }
};

class ParticleManager {
    std::vector<Particle> m_particles{};
    int m_framesUntilReset = 0;
    double m_spawnAccumulator = 0.0;

public:
    enum class Algorithm {
        Steering,
        Count,
    };

    struct AlgorithmParameters {
        int trailLength;
        float spawnRate;
        float particleSpeed;
        float particleAlpha;
        float minimumWindHeight = 0.05f;
        float maximumWindHeight = 1.0f;
        float steeringDistance = 28.0f;

        AlgorithmParameters() = default;

        AlgorithmParameters(int trailLength, float spawnRate, float particleSpeed, float particleAlpha = 0.85f) :
            trailLength(trailLength),
            spawnRate(spawnRate),
            particleSpeed(particleSpeed),
            particleAlpha(particleAlpha)
        {
        }
    };

    std::vector<AlgorithmParameters> parameters;

    ParticleManager() {
        parameters = std::vector<AlgorithmParameters>((size_t)Algorithm::Count);

        parameters[(size_t)Algorithm::Steering] = AlgorithmParameters(4, 5000.0f, 120.f, 1.0f);
    };

    void update(const cv::Mat& data, float deltaTime);

    void reset(int frames = 1)
    {
        m_framesUntilReset = std::max(m_framesUntilReset, std::max(frames, 1));
    }

    const std::vector<Particle>& getParticles() const
    {
        return m_particles;
    }

    size_t getParticleCount() const
    {
        return m_particles.size();
    }
};
