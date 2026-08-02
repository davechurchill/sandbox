#pragma once

#include "opencv2/core.hpp"

enum class Algorithms
{
	Average,
	HeatEquation,
    HeatEquationKernel,
    HeatEquationSIMD,
};

static std::vector<const char*> AlgorithmNames = {
	"Average",
	"Heat Equation",
    "Heat Equation Kernel",
    "Heat Equation SIMD",
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

};

class HeatGrid
{
	cv::Mat m_temps;
    cv::Mat m_workingTemps;
    cv::Mat m_normalized;
	bool    m_restartRequested = false;
    std::vector<HeatSource> m_sources;

public:

	Algorithms m_algorithm = Algorithms::HeatEquationSIMD;

    HeatGrid() = default;

    void update(const cv::Mat& kMat, int iterations);

    const cv::Mat& normalizedData() const
    {
        return m_normalized;
    }

	void reset()
	{
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

    const std::vector<HeatSource>& getSources() const
    {
        return m_sources;
    }

	void clearSources()
	{
        m_sources.clear();
	}


    void formulaAvg(const cv::Mat&);
    void formulaHeatParallel(const cv::Mat& kMat);
    void formulaHeatSIMD(const cv::Mat& kMat);
    void formulaHeatKernel(const cv::Mat& kMat);
	void updateSources();
};
