#pragma once

#include <opencv2/core.hpp>
#include <vector>

#include "SFML/System/Vector2.hpp"

enum class ParticleAlgorithm {
    CharneyEliassen = 0,
    BFS = 1,
};

struct Particle
{
    sf::Vector2<double> pos{ 0.0, 0.0 };
    std::vector<sf::Vector2<double>> trail{};

    Particle(double x, double y) : pos({ x, y })
    {
    }
};

class ParticleManager {
    std::vector<Particle> m_particles{};
    int m_framesUntilReset = 0;

public:
    int cellSize = 8;
    int trailLength = 4;
    int particleCount = 30000;
    float terrainWeight = 0.2f;
    float particleSpeed = 120.f;
    float particleAlpha = 0.85f;

    ParticleManager() = default;

    void update(ParticleAlgorithm algorithm, const cv::Mat& data, float deltaTime);

    void reset(int frames = 1)
    {
        m_framesUntilReset = std::max(m_framesUntilReset, std::max(frames, 1));
    }

    const std::vector<Particle>& getParticles() const
    {
        return m_particles;
    }
};
