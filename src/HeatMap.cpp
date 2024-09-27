#include "HeatMap.h"
#include <iostream>

namespace HeatMap
{
	void Grid::update(const cv::Mat& kMat)
	{
		const auto setSources = [&]()
		{
			for (auto& source : sources)
			{
				temps.at<float>(source.position) = source.getTempRaw();
			}
		};

		// Restart simulation if requested or if size has changed
		const cv::Size kMatSize = kMat.size();
		if (restartRequested || kMatSize != temps.size())
		{
			restartRequested = false;

			if (kMatSize.width <= 0 || kMatSize.height <= 0)
			{
				return;
			}

			// Boundaries are permanently at 0, all other cells are also initalized to 0
			temps = cv::Mat{ kMat.size(), CV_32F, 0.f };

			// Populate initial conditions
			setSources();
		}

		for (; stepsRequested > 0; stepsRequested--)
		{
			static cv::Mat workingTemps{};
			temps.copyTo(workingTemps);

			// Don't recompute at boundaries
			for (int i = 1; i < temps.rows - 1; i++)
			{
				for (int j = 1; j < temps.cols - 1; j++)
				{
					const float k = kMat.at<float>(i, j);
					workingTemps.at<float>(i, j) = getNewTemp(i, j, kMat);
				}
			}

			workingTemps.copyTo(temps);

			setSources();
		}
	}

	float Grid::getNewTemp(int i, int j, const cv::Mat& kMat)
	{
		switch (algorithm)
		{
		case Algorithms::Average:
		{
			const float sum =
				temps.at<float>(i - 1, j) +
				temps.at<float>(i + 1, j) +
				temps.at<float>(i, j - 1) +
				temps.at<float>(i, j + 1);

			return sum / 4;
		}
		case Algorithms::HeatEquation:
		{
			constexpr float dx = 1.f;
			constexpr float dy = 1.f;
			constexpr float dt = 0.25f;

			const float currCell = temps.at<float>(i, j);
			const float k = kMat.at<float>(i, j);

			const float hSum =
				temps.at<float>(i - 1, j) - 2 * currCell + temps.at<float>(i + 1, j);
			const float vSum =
				temps.at<float>(i, j - 1) - 2 * currCell + temps.at<float>(i, j + 1);

			return currCell + dt * k * (hSum / (dx * dx) + vSum / (dy * dy));
		}
		default:
			return temps.at<float>(i, j);
		};
	}
}