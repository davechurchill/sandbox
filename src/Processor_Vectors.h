#pragma once

#include "Profiler.hpp"
#include "SandboxProjector.h"
#include "Tools.h"
#include "TopographyProcessor.h"
#include "VectorField.h"
#include "GoodAssert.h"

struct Particle
{
    sf::Vector2<double> pos{ 0.0, 0.0 };
    std::vector<sf::Vector2<double>> trail{};

    Particle(double x, double y) : pos({ x, y })
    {
    }
};

class ParticleManager {
    int m_cellSize;
    int m_trailLength;
    VectorField m_field{};
    std::vector<Particle> m_particles{};

public:
    ParticleManager(int cellSize = 8, int trailLength = 4) : m_cellSize(cellSize), m_trailLength(trailLength)
    {
    }

    void createParticles(const cv::Mat& data, int particleCount = 30000)
    {
        m_particles.clear();
        m_particles.reserve(particleCount);
        for (int i = 0; i < particleCount; ++i)
        {
            m_particles.emplace_back(Particle{ (double)(rand() % data.cols), (double)(rand() % data.rows) });
        }
    }

    void update(const cv::Mat& data)
    {
        auto directions = m_field.compute(data, m_cellSize);

        int gridWidth = data.cols;
        int gridHeight = data.rows;

        auto clampPos = [&](sf::Vector2<double>& pos)
        {
            pos.x = std::max(0.0, std::min((double)(gridWidth - gridWidth % m_cellSize - 1), pos.x));
            pos.y = std::max(0.0, std::min((double)(gridHeight - gridHeight % m_cellSize - 1), pos.y));
        };

        for (auto& particle : m_particles)
        {
            // In case the grid size changed
            clampPos(particle.pos);

            for (auto& dir : directions.get(particle.pos.x / m_cellSize, particle.pos.y / m_cellSize))
            {
                particle.pos.x += dir.x;
                particle.pos.y += dir.y;
            }

            if (particle.pos.x >= gridWidth - m_cellSize * 2)
            {
                particle.pos.x = m_cellSize;
                particle.pos.y = rand() % gridHeight;
            }

            particle.trail.push_back({ particle.pos.x, particle.pos.y });
            if (particle.trail.size() > m_trailLength)
            {
                particle.trail.erase(particle.trail.begin());
            }

            // In case the particle went out of bounds
            clampPos(particle.pos);
        }
    }

    int trailLength() const
    {
        return m_trailLength;
    }

    const std::vector<Particle>& getParticles() const
    {
        return m_particles;
    }
};

class Processor_Vectors : public TopographyProcessor
{
    SandBoxProjector    m_projector;
    cv::Mat             m_cvTransformedDepthImage32f;
    sf::Image           m_sfTransformedDepthImage;
    sf::Texture         m_sfTransformedDepthTexture;
    sf::Sprite          m_sfTransformedDepthSprite;
    sf::Shader          m_shader;
    int                 m_selectedShaderIndex = 0;
    bool                m_drawContours = true;
    int                 m_numberOfContourLines = 19;
    ParticleManager     m_particleManager{};

public:
    void init();
    void imgui();
    void render(sf::RenderWindow& window);
    void processEvent(const sf::Event& event, const sf::Vector2f& mouse);
    void save(Save& save) const;
    void load(const Save& save);

    void processTopography(const cv::Mat& data);
};
