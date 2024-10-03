#pragma once

#include "opencv2/core.hpp"
#include <iostream>

constexpr float CelciusFactor = 100.f;
constexpr float KelvinDiff = 273.15f;

enum class Algorithms
{
	Average,
	HeatEquation
};

static const char* AlgorithmNames[] = {
	"Average",
	"Heat Equation"
};

struct HeatSource
{	
    float       m_temp;
	cv::Rect    m_area;

	HeatSource(const cv::Rect& area, const float temp) 
        : m_area(area)
        , m_temp(temp)
	{
			
	}

    bool contains(cv::Point point)
    {
        return m_area.contains(point);
    }
};

class HeatGrid
{
	cv::Mat m_temps;
    cv::Mat m_result;
    cv::Mat m_workingTemps;
    cv::Mat m_normalized;
	bool    m_restartRequested = false;
	int     m_iterations = 0;
    std::vector<HeatSource> m_sources;

public:

	Algorithms m_algorithm = Algorithms::HeatEquation;

    HeatGrid() = default;

    void update(const cv::Mat& kMat, int iterations);

	const cv::Mat& data() const
	{
		return m_temps;
	}

    const cv::Mat& normalizedData() const
    {
        return m_normalized;
    }

	void setIterations(int iterations)
	{
        m_iterations = iterations;
	}

	void reset()
	{
        m_iterations = 0;
        m_restartRequested = true;
	}

	void addSource(const HeatSource& source)
	{
        m_sources.push_back(source);
		updateSources();
	}

    std::vector<HeatSource>& getSources()
    {
        return m_sources;
    }

	void clearSources()
	{
        m_sources.clear();
	}


    void formulaAvg(const cv::Mat& kMat);
    void formulaHeat(const cv::Mat& kMat);
    void formulaHeatSIMD(const cv::Mat& kMat);
    void formulaHeatKernel(const cv::Mat& kMat);
	void updateSources();
};

