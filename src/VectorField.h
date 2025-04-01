#pragma once

#include <complex>
#include <fstream>
#include <opencv2/core.hpp>
#include <queue>

#include "Grid.hpp"
#include "SFML/System/Vector2.hpp"

namespace VectorField
{
    ////////////////////////
    // BFS
    ////////////////////////

    cv::Mat computeBFS(const cv::Mat& grid, int spacing, float terrainWeight);

    ////////////////////////
    // Charney
    ////////////////////////

    constexpr double PI = 3.1415926535897932384626433832;

    constexpr int SUMMATION_POINTS = 80;

    constexpr double PHI = PI / 4;
    constexpr double S = 2.5;
    constexpr double U = 0.29;
    constexpr double G = 9.8;
    const double OMEGA = 7.27 * std::pow(10, -5);

    constexpr double S_SQUARED = S * S;
    const double BETA = 4 * PI * std::pow(std::cos(PHI), 2);
    const double LAMBDA_SQUARED = 2.5 * std::pow(std::sin(2 * PHI), 2);
    const double M_SQUARED = BETA / U - S_SQUARED;
    const double F = 2 * OMEGA * sin(PHI);
    const double G_OVER_F = G / F;

    inline double greensTerm(double friction, int n, double nSquared_Minus_sSquared, double x);
    inline double greens(double friction, double x);

    class ComputeContext {
        void init(const cv::Mat& grid);

    public:
        const double friction;
        const double windVelocity;
        const double reductionFactor;

        int width = -1;
        int height = -1;
        double dx = 1;
        double dy = 1;
        double piOverWidth = 1;

        std::vector<double> x;
        std::vector<double> ySin;
        std::vector<double> h;

        cv::Mat zMat;
        cv::Mat uv;

        ComputeContext(const cv::Mat& grid, double friction, double windVelocity, double reductionFactor = 0.4);

        void computeWindTrajectories();
    };

    cv::Mat computePhysics(const cv::Mat& grid, bool compute);
}
