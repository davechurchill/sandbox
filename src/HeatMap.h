#pragma once

#include "opencv2/core.hpp"

namespace HeatMap
{
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

	class Grid
	{
		cv::Mat temps{};
		std::vector<HeatSource> sources{};
		bool restartRequested = false;
		bool stepRequested = false;

	public:
		Algorithms algorithm = Algorithms::HeatEquation;

		Grid(std::vector<HeatSource> sources = {}) : sources(sources) {}

		const cv::Mat& data() const
		{
			return temps;
		}

		void requestStep()
		{
			stepRequested = true;
		}

		void restart()
		{
			restartRequested = true;
		}

		void update(const cv::Mat& kMat);
		float getNewTemp(int i, int j, const cv::Mat& kMat = {});
	};
}
