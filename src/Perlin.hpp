#pragma once

#include <opencv2/core.hpp>

#include <cstdlib>
#include <vector>

class Perlin2DNew
{
    cv::Mat m_seed;

public:

    Perlin2DNew(size_t width = 512, size_t height = 512, int seed = 0)
        : m_seed(cv::Mat::zeros((int)height, (int)width, CV_32F))
    {
        srand(seed);
        for (size_t x = 0; x < width; x++)
        {
            for (size_t y = 0; y < height; y++)
            {
                m_seed.at<float>((int)y, (int)x) = (float)rand() / RAND_MAX;
            }
        }
    }

    float Interpolate(float x0, float x1, float alpha)
    {
        return x0 * (1 - alpha) + alpha * x1;
    }

    cv::Mat GeneratePerlinNoise(int octaveCount, float persistance)
    {
        const int width = m_seed.cols;
        const int height = m_seed.rows;

        std::vector<cv::Mat> smoothNoise;
        smoothNoise.reserve(octaveCount);

        //generate smooth noise
        for (int i = 0; i < octaveCount; i++)
        {
            smoothNoise.push_back(GenerateSmoothNoise(m_seed, i));
        }

        cv::Mat perlinNoise = cv::Mat::zeros(height, width, CV_32F);

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
                    perlinNoise.at<float>(j, i) += smoothNoise[octave].at<float>(j, i) * amplitude;
                }
            }
        }

        //normalisation
        for (int i = 0; i < width; i++)
        {
            for (int j = 0; j < height; j++)
            {
                perlinNoise.at<float>(j, i) /= totalAmplitude;
            }
        }

        return perlinNoise;
    }

    cv::Mat GenerateSmoothNoise(const cv::Mat & baseNoise, int octave)
    {
        const int width = m_seed.cols;
        const int height = m_seed.rows;

        cv::Mat smoothNoise = cv::Mat::zeros(height, width, CV_32F);

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
                float top = Interpolate(baseNoise.at<float>(sample_j0, sample_i0),
                    baseNoise.at<float>(sample_j0, sample_i1), horizontal_blend);

                //blend the bottom two corners
                float bottom = Interpolate(baseNoise.at<float>(sample_j1, sample_i0),
                    baseNoise.at<float>(sample_j1, sample_i1), horizontal_blend);

                //final blend
                smoothNoise.at<float>(j, i) = Interpolate(top, bottom, vertical_blend);
            }
        }

        return smoothNoise;
    }
};
