#pragma once

constexpr double M_PI = 3.1415926535897932384626433832;

#include <complex>
#include <fstream>
#include <opencv2/core.hpp>
#include <queue>

#include "Grid.hpp"
#include "SFML/System/Vector2.hpp"

namespace VectorField
{
    const int SUMMATION_POINTS = 80;

    const double PHI = M_PI / 4;
    const double S = 2.5;
    const double S2 = S * S;
    const double U = 0.29;

    const double BETA = 4 * M_PI * std::pow(std::cos(PHI), 2);
    const double LAMBDA2 = 2.5 * std::pow(std::sin(2 * PHI), 2);
    const double M2 = BETA / U - S2;

    const double G = 9.8;
    const double OMEGA = 7.27 * std::pow(10, -5);
    const double F = 2 * OMEGA * sin(PHI);
    const double G_OVER_F = G / F;

    using Matrix = std::vector<std::vector<double>>;

    Matrix zeroMatrix(int X_POINTS, int Y_POINTS) {
        return Matrix(Y_POINTS, std::vector<double>(X_POINTS, 0.0));
    }

    Grid<sf::Vector2<double>> computeBFS(const cv::Mat& grid, int spacing, float terrainWeight)
    {
        int pixelWidth = grid.cols;
        int pixelHeight = grid.rows;

        int gridWidth = pixelWidth / spacing;
        if (pixelWidth % spacing > 0) {
            gridWidth += 1;
        }

        int gridHeight = pixelHeight / spacing;
        if (pixelHeight % spacing > 0) {
            gridHeight += 1;
        }

        Grid<double> m_grid = Grid<double>(gridWidth, gridHeight, 0); // average terrain height
        Grid<double> m_distance = Grid<double>(gridWidth + 1, gridHeight, -1); // distance to nearest goal cell * (1 + terrain height * weight) 
        Grid<sf::Vector2<double>> m_directions = Grid<sf::Vector2<double>>(gridWidth, gridHeight, { 0, 0 }); // director vector

        for (int x = 0; x < gridWidth; ++x)
        {
            for (int y = 0; y < gridHeight; ++y)
            {
                int count = 0;
                double sum = 0;

                for (int sx = 0; sx < spacing && x * spacing + sx < pixelWidth; ++sx)
                {
                    for (int sy = 0; sy < spacing && y * spacing + sy < pixelHeight; ++sy)
                    {
                        count++;
                        sum += grid.at<float>(y * spacing + sy, x * spacing + sx);
                    }
                }

                m_grid.set(x, y, sum / count);
            }
        }

        std::queue<sf::Vector2<int>> openList{};

        for (int y = 0; y < gridHeight; ++y)
        {
            openList.push({ gridWidth, y });
            m_distance.set(gridWidth, y, 0);
        }

        while (!openList.empty())
        {
            sf::Vector2i cell = openList.front();
            openList.pop();

            auto value = m_distance.get(cell.x, cell.y);

            auto update = [&](int x, int y)
            {
                auto& dist = m_distance.get(x, y);
                if (dist == -1)
                {
                    openList.push({ x, y });
                    dist = value + 1;
                }
            };

            if (cell.x > 0)
            {
                update(cell.x - 1, cell.y);
            }
            if (cell.x < gridWidth)
            {
                update(cell.x + 1, cell.y);
            }
            if (cell.y > 0)
            {
                update(cell.x, cell.y - 1);
            }
            if (cell.y < gridHeight - 1)
            {
                update(cell.x, cell.y + 1);
            }
        }

        for (int x = 0; x < gridWidth; ++x)
        {
            for (int y = 0; y < gridHeight; ++y)
            {
                m_distance.get(x, y) *= 1 + m_grid.get(x, y) * terrainWeight;
            }
        }

        for (int x = 0; x < gridWidth; ++x)
        {
            for (int y = 0; y < gridHeight; ++y)
            {
                auto thisDist = m_distance.get(x, y);

                double left = x > 0 ? m_distance.get(x - 1, y) : thisDist;
                double right = x < gridWidth - 1 ? m_distance.get(x + 1, y) : thisDist;
                double up = y > 0 ? m_distance.get(x, y - 1) : thisDist;
                double down = y < gridHeight - 1 ? m_distance.get(x, y + 1) : thisDist;

                sf::Vector2<double> dir{ left - right, up - down };

                double magnitude = std::sqrt(dir.x * dir.x + dir.y * dir.y);

                m_directions.get(x, y) = { dir.x / magnitude, dir.y / magnitude };
            }
        }

        return m_directions;
    }

    double greens_term(int n, double n2, double friction, double x)
    {
        std::complex<double> numerator = std::exp(std::complex<double>(0, n * x));
        std::complex<double> denominator = n2 - S2 - std::complex<double>(0, friction * (n + M2) / n);
        return (numerator / denominator).real();
    }

    double greens(double friction, double x)
    {
        double sum = 0;

        for (int n = 1; n <= SUMMATION_POINTS / 2; ++n)
        {
            double n2 = n * n;
            sum += greens_term(n, n2, friction, x) + greens_term(-n, n2, friction, x);
        }

        return sum / (2.0 * M_PI);
    }

    double integrate(int X_POINTS, const std::vector<double>& y)
    {
        double integral = 0.0;

        for (size_t i = 1; i < X_POINTS; ++i)
        {
            integral += 2 * M_PI / X_POINTS * (y[i] + y[i - 1]);
        }

        return integral / 2.0;
    }

    std::vector<double> z(int X_POINTS, std::vector<double>& X, double friction, std::vector<double>& h, double reduction_factor = 0.4)
    {
        std::vector<double> result(X_POINTS);

        for (size_t i = 0; i < X_POINTS; i++)
        {
            std::vector<double> integrand(X_POINTS);

            for (size_t j = 0; j < X_POINTS; j++)
            {
                integrand[j] = h[j] * greens(friction, X[i] - X[j]);
            }

            result[i] = reduction_factor * LAMBDA2 * integrate(X_POINTS, integrand);
        }

        return result;
    }

    void computeWindTrajectories(int X_POINTS, int Y_POINTS, std::vector<double>& X, std::vector<double>& SIN_Y, double DX, double DY, Matrix& u, Matrix& v, double friction, std::vector<double>& h, double wind_velocity)
    {
        // Compute z Matrix

        auto z_values = z(X_POINTS, X, friction, h);
        Matrix z_mat = zeroMatrix(X_POINTS, Y_POINTS);

        for (int i = 0; i < Y_POINTS; ++i)
        {
            for (int j = 0; j < X_POINTS; ++j)
            {
                z_mat[i][j] = z_values[j] * SIN_Y[i];
            }
        }

        // Compute raw trajectories

#define compute_v(a, b) (z_mat[i][a] - z_mat[i][b]) / DX * G_OVER_F

        for (int i = 1; i < Y_POINTS - 1; ++i)
        {
            for (int j = 0; j < X_POINTS; ++j)
            {
                u[i][j] = (z_mat[i + 1][j] - z_mat[i - 1][j]) / DY * -G_OVER_F;
            }

            for (int j = 1; j < X_POINTS - 1; ++j)
            {
                v[i][j] = compute_v(j + 1, j - 1);
            }

            // Handle edge cases
            v[i][0] = compute_v(0, X_POINTS - 1);
            v[i][X_POINTS - 1] = compute_v(X_POINTS - 1, 0);
        }

        // Normalize u and v and apply constant velocity

        double normal_factor = 0.0;

        for (int i = 0; i < Y_POINTS; ++i)
        {
            for (int j = 0; j < X_POINTS; ++j)
            {
                normal_factor = std::max(normal_factor, std::max(std::abs(u[i][j]), std::abs(v[i][j])));
            }
        }

        for (int i = 0; i < Y_POINTS; ++i)
        {
            for (int j = 0; j < X_POINTS; ++j)
            {
                u[i][j] = u[i][j] / normal_factor + wind_velocity;
                v[i][j] = v[i][j] / normal_factor;
            }
        }
    }

    Grid<sf::Vector2<double>> computePhysics(const cv::Mat& grid, bool compute)
    {
        const int X_POINTS = grid.cols;
        const int Y_POINTS = grid.rows;

        static Grid<sf::Vector2<double>> directions = Grid<sf::Vector2<double>>(X_POINTS, Y_POINTS, { 0, 0 });

        if (compute) {
            const double DX = 2 * M_PI / X_POINTS;
            const double DY = M_PI / Y_POINTS;

            std::vector<double> X(X_POINTS);
            std::vector<double> SIN_Y(Y_POINTS);
            std::vector<double> H(X_POINTS);

            // Set up vectors

            for (int j = 0; j < X_POINTS; ++j)
            {
                const double x = j * DX;
                X[j] = x;

                double sum = 0.0;

                for (int i = 0; i < Y_POINTS; ++i)
                {
                    sum += grid.at<double>(i, j);
                }

                H[j] = sum / Y_POINTS;
            }

            for (int i = 0; i < Y_POINTS; ++i)
            {
                const double y = i * DY;
                SIN_Y[i] = sin(i * DY);
            }

            Matrix u = zeroMatrix(X_POINTS, Y_POINTS);
            Matrix v = zeroMatrix(X_POINTS, Y_POINTS);
            computeWindTrajectories(X_POINTS, Y_POINTS, X, SIN_Y, DX, DY, u, v, 0.5, H, 0.4);

            for (size_t x = 0; x < X_POINTS; ++x)
            {
                for (size_t y = 0; y < Y_POINTS; ++y)
                {
                    directions.set(x, y, { u[y][x], v[y][x] });
                }
            }
        }

        return directions;
    }
}
