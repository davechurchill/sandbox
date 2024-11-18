#pragma once

#include <opencv2/core.hpp>
#include <vector>

#include "SFML/System/Vector2.hpp"

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
    bool m_resetRequested = false;

public:
    int cellSize = 8;
    int trailLength = 4;
    int particleCount = 30000;
    float terrainWeight = 0.2f;
    float particleSpeed = 120.0;

    ParticleManager() = default;

    void update(const cv::Mat& data, float deltaTime);

    void reset()
    {
        m_resetRequested = true;
    }

    const std::vector<Particle>& getParticles() const
    {
        return m_particles;
    }
};
