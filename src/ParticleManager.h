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
    int m_framesUntilReset = 0;

    bool checkSimilar(const cv::Mat& mat1, const cv::Mat& mat2, double tolerance = 0.1);

public:
    enum class Algorithm {
        CharneyEliassen,
        BFS,
        Count,
    };

    static const char* AlgorithmNames[];

    struct AlgorithmParameters {
        int cellSize;
        int trailLength;
        int particleCount;
        float terrainWeight;
        float particleSpeed;
        float particleAlpha;

        AlgorithmParameters() = default;

        AlgorithmParameters(int trailLength, int particleCount, float particleSpeed, int cellSize = 8, float terrainWeight = 0.2f, float particleAlpha = 0.85f) :
            trailLength(trailLength),
            particleCount(particleCount),
            particleSpeed(particleSpeed),
            particleAlpha(particleAlpha),
            cellSize(cellSize),
            terrainWeight(terrainWeight)
        {
        }
    };

    std::vector<AlgorithmParameters> parameters;

    ParticleManager() {
        parameters = std::vector<AlgorithmParameters>((size_t)Algorithm::Count);

        parameters[(size_t)Algorithm::CharneyEliassen] = AlgorithmParameters(8, 4000, 160.f);
        parameters[(size_t)Algorithm::BFS] = AlgorithmParameters(4, 30000, 120.f);
    };

    void update(Algorithm algorithm, const cv::Mat& data, float deltaTime);

    void reset(int frames = 1)
    {
        m_framesUntilReset = std::max(m_framesUntilReset, std::max(frames, 1));
    }

    const std::vector<Particle>& getParticles() const
    {
        return m_particles;
    }
};
