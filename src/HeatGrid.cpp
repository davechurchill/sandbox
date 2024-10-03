#include "HeatGrid.h"
#include <iostream>
#include <opencv2/core.hpp> // Ensure core functionalities are included
#include <opencv2/imgproc/imgproc.hpp>

void setRectValue(cv::Mat& mat, const cv::Rect& rect, float value)
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
    if (m_temps.size() != kMat.size())
    {
        m_temps = cv::Mat(kMat.size(), CV_32F, 0.f);
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
            formulaHeat(kMat);
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
        setRectValue(m_temps, source.m_area, source.m_temp);
    }
}

void HeatGrid::formulaAvg(const cv::Mat& kMat)
{
    constexpr float dt = 0.25f;
    cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
        0.00, 0.25, 0.00,
        0.25, 0.00, 0.25,
        0.00, 0.25, 0.00);

    cv::filter2D(m_temps, m_workingTemps, CV_32F, kernel);
    m_workingTemps.copyTo(m_temps);
    updateSources();
}

void HeatGrid::formulaHeat(const cv::Mat& kMat)
{
    static cv::Mat workingTemps{};
    
    // Use OpenCV's parallel_for_ for parallel processing
    cv::Range range(1, m_temps.rows - 1);
    cv::parallel_for_(range, [&](const cv::Range& r) {
        for (int i = r.start; i < r.end; i++)
        {
            for (int j = 1; j < m_temps.cols - 1; j++)
            {
                constexpr float dt = 0.25f;
                float& cell = m_temps.at<float>(i, j);
                float& newCell = m_workingTemps.at<float>(i, j);
                float k = kMat.at<float>(i, j);
                k = k * k * k;

                const float neighBourSum =
                    m_temps.at<float>(i - 1, j) +
                    m_temps.at<float>(i + 1, j) +
                    m_temps.at<float>(i, j - 1) +
                    m_temps.at<float>(i, j + 1);

                newCell = cell + dt * k * (neighBourSum - 4 * cell);
            }
        }
        });

    m_workingTemps.copyTo(m_temps);
    updateSources();
}
