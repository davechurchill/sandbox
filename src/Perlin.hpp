#pragma once

#include <vector>

class Perlin1D
{

    std::vector<float> m_seed;
    std::vector<float> m_output;

    std::vector<std::vector<float>> m_octaves;

public:

    Perlin1D(size_t size = 256, int seed = 0)
        : m_seed(size)
        , m_output(size)
    {
        srand(seed);
        for (size_t i = 0; i < size; i++)
        {
            m_seed[i] = (float)rand() / RAND_MAX;
        }
    }

    void calculate(int octaves, float bias)
    {
        m_output.clear();

        for (int x = 0; x < (int)m_seed.size(); x++)
        {
            float noise = 0.0f;
            float scale = 1.0f;
            float scaleSum = 0.0f;

            for (int o = 0; o < octaves; o++)
            {
                int pitch = (int)m_seed.size() >> o;
                int sample1 = (x / pitch) * pitch;
                int sample2 = (sample1 + pitch) % m_seed.size();

                float blend = (float)(x - sample1) / (float)pitch;
                float sample = (1.0f - blend) * m_seed[sample1] + blend * m_seed[sample2];
                noise += sample * scale;
                scaleSum += scale;
                scale /= bias;
            }

            m_output[x] = noise / scaleSum;
        }
    }

    const std::vector<float> getSeed() const
    {
        return m_seed;
    }

    const std::vector<float> getOutput() const
    {
        return m_output;
    }
};

class Perlin2DNew
{
    Grid<float> m_seed;
    Grid<float> m_output;

public:

    Perlin2DNew(size_t width = 512, size_t height = 512, int seed = 0)
        : m_seed(width, height, 0)
    {
        srand(seed);
        for (size_t x = 0; x < width; x++)
        {
            for (size_t y = 0; y < height; y++)
            {
                m_seed.set(x, y, (float)rand() / RAND_MAX);
            }
        }
    }

    float Interpolate(float x0, float x1, float alpha)
    {
        return x0 * (1 - alpha) + alpha * x1;
    }

    Grid<float> GeneratePerlinNoise(int octaveCount, float persistance)
    {
        int width = (int)m_seed.width();
        int height = (int)m_seed.height();

        std::vector<Grid<float>> smoothNoise(octaveCount, Grid<float>(width, height, 0));

        //generate smooth noise
        for (int i = 0; i < octaveCount; i++)
        {
            smoothNoise[i] = GenerateSmoothNoise(m_seed, i);
        }

        Grid<float> perlinNoise(width, height, 0);

        float amplitude = 1.0f;
        float totalAmplitude = 0.0f;

        //blend noise together
        for (int octave = octaveCount - 1; octave >= 0; octave--)
        {
            amplitude *= persistance;
            totalAmplitude += amplitude;

            for (int i = 0; i < width; i++)
            {
                for (int j = 0; j < height; j++)
                {
                    perlinNoise.add(i, j, smoothNoise[octave].get(i, j) * amplitude);
                }
            }
        }

        //normalisation
        for (int i = 0; i < width; i++)
        {
            for (int j = 0; j < height; j++)
            {
                perlinNoise.set(i, j, perlinNoise.get(i, j) / totalAmplitude);
            }
        }

        return perlinNoise;
    }

    Grid<float> GenerateSmoothNoise(Grid<float> & baseNoise, int octave)
    {
        int width = (int)m_seed.width();
        int height = (int)m_seed.height();

        Grid<float> smoothNoise(width, height, 0);

        int samplePeriod = 1 << octave; // calculates 2 ^ k
        float sampleFrequency = 1.0f / samplePeriod;

        for (int i = 0; i < width; i++)
        {
            //calculate the horizontal sampling indices
            int sample_i0 = (i / samplePeriod) * samplePeriod;
            int sample_i1 = (sample_i0 + samplePeriod) % width; //wrap around
            float horizontal_blend = (i - sample_i0) * sampleFrequency;

            for (int j = 0; j < height; j++)
            {
                //calculate the vertical sampling indices
                int sample_j0 = (j / samplePeriod) * samplePeriod;
                int sample_j1 = (sample_j0 + samplePeriod) % height; //wrap around
                float vertical_blend = (j - sample_j0) * sampleFrequency;

                //blend the top two corners
                float top = Interpolate(baseNoise.get(sample_i0, sample_j0),
                    baseNoise.get(sample_i1, sample_j0), horizontal_blend);

                //blend the bottom two corners
                float bottom = Interpolate(baseNoise.get(sample_i0, sample_j1),
                    baseNoise.get(sample_i1, sample_j1), horizontal_blend);

                //final blend
                smoothNoise.set(i, j, Interpolate(top, bottom, vertical_blend));
            }
        }

        return smoothNoise;
    }

    const Grid<float> getSeed() const
    {
        return m_seed;
    }

    const Grid<float> getOutput() const
    {
        return m_output;
    }
};

class Perlin2D
{
    Grid<float> m_seed;
    Grid<float> m_output;

    std::vector<std::vector<float>> m_octaves;

public:

    Perlin2D(size_t width = 256, size_t height = 256, int seed = 0)
        : m_seed(width, height, 0)
    {
        srand(seed);
        for (size_t x = 0; x < width; x++)
        {
            for (size_t y = 0; y < height; y++)
            {
                m_seed.set(x,y,(float)rand() / RAND_MAX);
            }
        }
    }

    void calculate(int width, int height, int octaves, float bias)
    {
        m_output = Grid<float>(width, height, 0);

        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                float noise = 0.0f;
                float scale = 1.0f;
                float scaleSum = 0.0f;

                for (int o = 0; o < octaves; o++)
                {
                    int pitch = width >> o;
                    int sampleX1 = (x / pitch) * pitch;
                    int sampleY1 = (y / pitch) * pitch;
                    int sampleX2 = (sampleX1 / pitch) % width;
                    int sampleY2 = (sampleY1 / pitch) % width;

                    float blendX = (float)(x - sampleX1) / (float)pitch;
                    float blendY = (float)(x - sampleY1) / (float)pitch;
                    float sampleT = (1.0f - blendX) * m_seed.get(sampleX1, sampleY1) + blendX * m_seed.get(sampleX2, sampleY1);
                    float sampleB = (1.0f - blendX) * m_seed.get(sampleX1, sampleY2) + blendX * m_seed.get(sampleX2, sampleY2);
                    noise += (blendY * (sampleB - sampleT) + sampleT) * scale;
                    scaleSum += scale;
                    scale /= bias;
                }

                m_output.set(x, y, noise / scaleSum);
            }
        }
    }

    const Grid<float> getSeed() const
    {
        return m_seed;
    }

    const Grid<float> getOutput() const
    {
        return m_output;
    }
};