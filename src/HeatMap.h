#pragma once

#include "opencv2/core.hpp"

namespace
{
	constexpr float CelciusFactor = 100.f;
	constexpr float KelvinDiff = 273.15f;
}

class HeatSource
{
	// All temperatures are celcius, in the range [0.0, 100.0]
	// However, they are stored as [0.0, 1.0] for precision
	float temp;

public:
	cv::Point position;

	HeatSource(const cv::Point& position, const float temp) : position(position)
	{
		setTempCelcius(temp);
	}

	void setTempCelcius(const float temp)
	{
		this->temp = temp / CelciusFactor;
	}

	float getTempCelcius() const
	{
		return temp * CelciusFactor;
	}

	void setTempKelvin(const float temp)
	{
		this->temp = (temp + KelvinDiff) / CelciusFactor;
	}

	float getTempKelvin() const
	{
		return (temp * CelciusFactor) - KelvinDiff;
	}

	float getTempRaw() const
	{
		return temp;
	}
};

class HeatMap
{
	cv::Mat temps{};

	std::vector<HeatSource> sources{};

	bool stepRequested = false;

public:
	HeatMap(std::vector<HeatSource> sources = {}) : sources(sources) {}

	const cv::Mat& data() const
	{
		return temps;
	}

	void requestStep()
	{
		stepRequested = true;
	}

	void update(const cv::Mat& kMat);
};
