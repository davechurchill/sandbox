#include <iostream>
#include <SFML/System/Time.hpp>

#include "ParticleManager.h"
#include "VectorField.hpp"

namespace {
    bool checkSimilar(const cv::Mat& mat1, const cv::Mat& mat2, double tolerance = 0.1) {
        if (mat1.size() != mat2.size())
        {
            return false;
        }

        double error = cv::norm(mat1, mat2, cv::NORM_L2);
        double normFactor = cv::norm(mat1, cv::NORM_L2);

        if (normFactor > 0)
        {
            error /= normFactor;
        }

        return error < tolerance;
    }
}

void ParticleManager::update(ParticleAlgorithm algorithm, const cv::Mat& data, float deltaTime)
{
    const int pixelWidth = data.cols;
    const int pixelHeight = data.rows;

    Grid<sf::Vector2<double>> directions;

    static ParticleAlgorithm previousAlgorithm = algorithm;

    static double computeTimer = 0.0;
    computeTimer -= deltaTime;

    if (algorithm != previousAlgorithm)
    {
        previousAlgorithm = algorithm;
        computeTimer = 0.0;
        reset();
    }

    switch (algorithm)
    {
    case ParticleAlgorithm::BFS: {
        directions = VectorField::computeBFS(data, cellSize, terrainWeight);
        break;
    }
    case ParticleAlgorithm::CharneyEliassen: {
        static cv::Mat oldData = cv::Mat();

        bool compute = false;

        if (computeTimer <= 0.0 && !checkSimilar(oldData, data)) {
            oldData = data.clone();
            compute = true;
            reset(2);
        }

        directions = VectorField::computePhysics(data, compute);

        break;
    }
    default: return; // Unknown algorithm, return
    }

    if (computeTimer <= 0.0) {
        computeTimer = m_computeFrequency;
    }

    if (m_particles.size() != particleCount)
    {
        m_particles.clear();
        m_particles.reserve(particleCount);

        for (int i = 0; i < particleCount; ++i)
        {
            m_particles.emplace_back(Particle{ (double)(rand() % pixelWidth), (double)(rand() % pixelHeight) });
        }
    }

    if (m_framesUntilReset > 0)
    {
        m_framesUntilReset--;

        if (m_framesUntilReset == 0)
        {
            for (auto& particle : m_particles)
            {
                particle.pos.x = rand() % pixelWidth;
                particle.pos.y = rand() % pixelHeight;
            }
        }
    }

    auto clampPos = [&](sf::Vector2<double>& pos)
    {
        // TODO: Figure out why particles get stuck on the edges
        pos.x = std::clamp(pos.x, 0.0, (double)(pixelWidth - 1));
        pos.y = std::clamp(pos.y, 0.0, (double)(pixelHeight - 1));
    };

    for (auto& particle : m_particles)
    {
        // In case the grid size changed
        clampPos(particle.pos);

        sf::Vector2<double> particlePos = { particle.pos.x, particle.pos.y };

        // Only BFS uses cellSize
        if (algorithm == ParticleAlgorithm::BFS) {
            particlePos.x /= cellSize;
            particlePos.y /= cellSize;
        }

        auto& dir = directions.get((size_t)particlePos.x, (size_t)particlePos.y);

        particle.pos.x += dir.x * particleSpeed * deltaTime;
        particle.pos.y += dir.y * particleSpeed * deltaTime;

        if (particle.pos.x >= pixelWidth - 1)
        {
            particle.pos.x = std::fmod(particle.pos.x, (double)(pixelWidth - 1));
            particle.pos.y = rand() % pixelHeight;
            particle.trail.clear();
        }

        particle.trail.push_back({ particle.pos.x, particle.pos.y });
        while (particle.trail.size() > trailLength)
        {
            particle.trail.erase(particle.trail.begin());
        }

        // In case the particle or its trails went out of bounds
        clampPos(particle.pos);
        for (auto& segment : particle.trail)
        {
            clampPos(segment);
        }
    }
}
