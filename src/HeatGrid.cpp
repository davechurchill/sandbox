#include "HeatGrid.h"
#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <immintrin.h>

namespace
{
    void ZeroBoundary(cv::Mat& matrix)
    {
        if (matrix.empty()) { return; }

        matrix.row(0).setTo(0.0f);
        if (matrix.rows > 1) { matrix.row(matrix.rows - 1).setTo(0.0f); }
        matrix.col(0).setTo(0.0f);
        if (matrix.cols > 1) { matrix.col(matrix.cols - 1).setTo(0.0f); }
    }
}


void SetRectValue(cv::Mat& mat, const cv::Rect& rect, float value)
{
    // Check if the input matrix is valid
    if (mat.empty()) { return; }

    // Calculate the valid rectangle
    int x1 = std::max(0, rect.x);
    int y1 = std::max(0, rect.y);
    int x2 = std::min(mat.cols, rect.x + rect.width);
    int y2 = std::min(mat.rows, rect.y + rect.height);

    // Create a bounded rectangle
    cv::Rect boundedRect(x1, y1, x2 - x1, y2 - y1);

    // Check if the bounded rectangle is valid
    if (boundedRect.width <= 0 || boundedRect.height <= 0) { return; }

    // Set all values within the bounded rectangle to the specified value
    mat(boundedRect).setTo(value);
}

void HeatGrid::update(const cv::Mat& kMat, int iterations)
{
    const cv::Size kMatSize = kMat.size();
    if (kMatSize.width <= 0 || kMatSize.height <= 0) { return; }

    // Boundaries are permanently at 0, all other cells are also initialized to 0
    if (m_restartRequested || m_temps.size() != kMat.size())
    {
        m_temps = cv::Mat(kMat.size(), CV_32F, 0.f);
        m_restartRequested = false;
    }

    // make sure heat sources are in their place
    updateSources();

    // create the temporary matrix we can work with
    m_temps.copyTo(m_workingTemps);

    for (int iter = 0; iter < iterations; iter++)
    {
        if (m_algorithm == Algorithms::Average)
        {   
            formulaAvg(kMat);
        }
        else if (m_algorithm == Algorithms::HeatEquation)
        {
            formulaHeatParallel(kMat);
        }
        else if (m_algorithm == Algorithms::HeatEquationKernel)
        {
            formulaHeatKernel(kMat);
        }
        else if (m_algorithm == Algorithms::HeatEquationSIMD)
        {
            formulaHeatSIMD(kMat);
        }
    }

    // normalize the data between a given min and max temperature
    // 0.5 in the normalized data will be equivalent to 0 for visualization
    const double maxVal = 100;       // this will be 1
    const double minVal = -maxVal;   // this will be 0
    m_normalized = (m_temps - minVal) / (maxVal - minVal);
}

void HeatGrid::updateSources()
{
    for (auto& source : m_sources)
    {
        SetRectValue(m_temps, source.m_area, source.m_temp);
    }
    ZeroBoundary(m_temps);
}

void HeatGrid::formulaAvg(const cv::Mat&)
{
    cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
        0.00, 0.25, 0.00,
        0.25, 0.00, 0.25,
        0.00, 0.25, 0.00);

    cv::filter2D(m_temps, m_workingTemps, CV_32F, kernel);
    m_workingTemps.copyTo(m_temps);
    updateSources();
}

void HeatGrid::formulaHeatParallel(const cv::Mat& kMat)
{
    const int rows = m_temps.rows;
    const int cols = m_temps.cols;

    cv::Range range(1, rows - 1);
    cv::parallel_for_(range, [&](const cv::Range& r) 
    {
        constexpr float dt = 0.25f;

        // Get the step sizes (number of elements per row)
        size_t tempsStep        = m_temps.step1();
        size_t workingTempsStep = m_workingTemps.step1();
        size_t kMatStep         = kMat.step1();

        // Get pointers to the data
        float* tempsData        = m_temps.ptr<float>();
        float* workingTempsData = m_workingTemps.ptr<float>();
        float* kMatData         = ((cv::Mat&)kMat).ptr<float>();

        for (int i = r.start; i < r.end; ++i)
        {
            // Pointers to the current and neighboring rows
            float* currRow = tempsData +  i      * tempsStep;
            float* prevRow = tempsData + (i - 1) * tempsStep;
            float* nextRow = tempsData + (i + 1) * tempsStep;

            float* kRow       = kMatData + i * kMatStep;
            float* workingRow = workingTempsData + i * workingTempsStep;

            for (int j = 1; j < cols - 1; ++j)
            {
                float cell = currRow[j];
                float k = kRow[j];
                k = k * k * k; // Compute k^3

                // Sum of neighboring cells
                float neighbourSum = prevRow[j] + nextRow[j] + currRow[j - 1] + currRow[j + 1];

                // Update the working temperature
                workingRow[j] = cell + dt * k * (neighbourSum - 4 * cell);
            }
        }
    });

    // Swap the matrices instead of copying to avoid unnecessary memory operations
    std::swap(m_temps, m_workingTemps);
    updateSources();
}


void HeatGrid::formulaHeatSIMD(const cv::Mat& kMat)
{
    constexpr float dt = 0.25f;
    const int rows = m_temps.rows;
    const int cols = m_temps.cols;

    __m256 dtVec = _mm256_set1_ps(dt);
    __m256 fourVec = _mm256_set1_ps(4.0f);

    cv::parallel_for_(cv::Range(1, rows - 1), [&](const cv::Range& range)
    {
        for (int i = range.start; i < range.end; ++i)
        {
            float* currRow   = m_temps.ptr<float>(i);
            float* prevRow   = m_temps.ptr<float>(i - 1);
            float* nextRow   = m_temps.ptr<float>(i + 1);
            float* resultRow = m_workingTemps.ptr<float>(i);
            float* kRow      = ((cv::Mat&)kMat).ptr<float>(i);

            int j = 1;
            for (; j <= cols - 9; j += 8)
            {
                __m256 cell  = _mm256_loadu_ps(currRow + j);
                __m256 north = _mm256_loadu_ps(prevRow + j);
                __m256 south = _mm256_loadu_ps(nextRow + j);
                __m256 west  = _mm256_loadu_ps(currRow + j - 1);
                __m256 east  = _mm256_loadu_ps(currRow + j + 1);

                __m256 neighborSum = _mm256_add_ps(_mm256_add_ps(north, south), _mm256_add_ps(west, east));
                __m256 laplacian   = _mm256_sub_ps(neighborSum, _mm256_mul_ps(fourVec, cell));

                __m256 k = _mm256_loadu_ps(&kRow[j]);
                        k = _mm256_mul_ps(k, _mm256_mul_ps(k, k)); // k^3

                __m256 newCell = _mm256_fmadd_ps(_mm256_mul_ps(dtVec, k), laplacian, cell);

                _mm256_storeu_ps(&resultRow[j], newCell);
            }

            // Handle remaining columns
            for (; j < cols - 1; ++j)
            {
                float laplacian = prevRow[j] + nextRow[j] + currRow[j - 1] + currRow[j + 1] - 4 * currRow[j];
                float k = kRow[j];
                k = k * k * k;

                resultRow[j] = currRow[j] + dt * k * laplacian;
            }
        }
    });

    // Swap matrices to avoid copying
    std::swap(m_temps, m_workingTemps);
    updateSources();
}


void HeatGrid::formulaHeatKernel(const cv::Mat& kMat)
{
    constexpr float dt = 0.25f;
    // Use cv::Laplacian for efficiency
    cv::Laplacian(m_temps, m_workingTemps, CV_32F);

    // Match the conductivity used by the other heat-equation implementations.
    cv::Mat conductivity;
    cv::multiply(kMat, kMat, conductivity);
    cv::multiply(conductivity, kMat, conductivity);
    cv::multiply(m_workingTemps, conductivity, m_workingTemps, dt);

    // Update m_temps directly
    cv::add(m_temps, m_workingTemps, m_temps);

    updateSources();
}
