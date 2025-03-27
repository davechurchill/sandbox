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

    static ParticleAlgorithm previousAlgorithm = algorithm;
    if (algorithm != previousAlgorithm)
    {
        previousAlgorithm = algorithm;
        reset();
    }

    // TODO: DYNAMIC! BAD! BAD! BAD!
    cv::Mat directions;

    switch (algorithm)
    {
    case ParticleAlgorithm::BFS: {
        directions = VectorField::computeBFS(data, cellSize, terrainWeight);
        break;
    }
    case ParticleAlgorithm::CharneyEliassen: {
        static cv::Mat oldData = cv::Mat();

        bool compute = false;

        static double timer = 0.0;
        bool timerTriggered = false;
        timer -= deltaTime;
        if (timer <= 0)
        {
            timer = 2.0;
            timerTriggered = true;
        }

        const bool sameSize = oldData.size() == data.size();
        const bool sameData = checkSimilar(oldData, data);

        if (timerTriggered || !sameSize || !sameData) {
            oldData = data.clone();
            compute = true;
        }

        if (!sameSize)
        {
            reset(3);
        }

        directions = VectorField::computePhysics(data, compute);

        break;
    }
    default:
        directions = cv::Mat::zeros(data.cols, data.rows, CV_64FC2);
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

        // TODO: Is (int) this the best way to round here?
        auto& dir = directions.at<cv::Vec2d>((int)particlePos.y, (int)particlePos.x);

        particle.pos.x += dir[0] * particleSpeed * deltaTime;
        particle.pos.y += dir[1] * particleSpeed * deltaTime;

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
