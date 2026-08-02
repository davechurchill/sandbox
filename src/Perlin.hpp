#pragma once

#include <Grid.hpp>

#include <vector>

class Perlin2DNew
{
    Grid<float> m_seed;

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
};
