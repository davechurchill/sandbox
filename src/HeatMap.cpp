#include "HeatMap.h"

namespace
{
    enum class ComputeMethod {
        Average,
        HeatEquation
    };

    const ComputeMethod method = ComputeMethod::HeatEquation;

    float computeAverage(const cv::Mat& temps, int i, int j)
    {
        const float currCell = temps.at<float>(i, j);
        float sum = currCell;

        if (i > 0)
        {
            sum += temps.at<float>(i - 1, j);
        }
        if (i < temps.cols - 1)
        {
            sum += temps.at<float>(i + 1, j);
        }

        if (j > 0)
        {
            sum += temps.at<float>(i, j - 1);
        }
        if (j < temps.rows - 1)
        {
            sum += temps.at<float>(i, j + 1) ;
        }

        return sum / 5;
    }

    float computeHeatEquation(const cv::Mat& temps, int i, int j)
    {
        const float currCell = temps.at<float>(i, j);
        float sum = currCell;

        if (i > 0)
        {
            sum += temps.at<float>(i - 1, j) - currCell;
        }
        if (i < temps.rows - 1)
        {
            sum += temps.at<float>(i + 1, j) - currCell;
        }

        if (j > 0)
        {
            sum += temps.at<float>(i, j - 1) - currCell;
        }
        if (j < temps.cols - 1)
        {
            sum += temps.at<float>(i, j + 1) - currCell;
        }

        return sum;
    }
}

void HeatMap::update(const cv::Mat& kMat)
{
    // Restart simulation if size has changed
    
    const cv::Size kMatSize = kMat.size();

	if (kMatSize != temps.size())
	{
        if (kMatSize.width <= 0 || kMatSize.height <= 0)
        {
            return;
        }

        temps = cv::Mat{ kMat.size(), CV_32F, 0.f };
	}

	if (stepRequested)
	{
		stepRequested = false;

        static cv::Mat workingTemps{};

        temps.copyTo(workingTemps);

        for (int i = 0; i < temps.rows; i++)
        {
            for (int j = 0; j < temps.cols; j++)
            {
                static auto getNewTemp = [&](int i, int j)
                {
                    switch (method)
                    {
                    case ComputeMethod::Average: return computeAverage(temps, i, j);
                    case ComputeMethod::HeatEquation: return computeHeatEquation(temps, i, j);
                    default: return temps.at<float>(i, j);
                    };
                };

                workingTemps.at<float>(i, j) = getNewTemp(i, j);
            }
        }

        workingTemps.copyTo(temps);

        for (auto& source : sources)
        {
            temps.at<float>(source.position) = source.getTempRaw();
        }
	}
}
