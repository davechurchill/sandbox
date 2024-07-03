#pragma once

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API

class CameraFilters
{
    int                 m_decimation = 1;
    rs2::decimation_filter m_decimationFilter;

    float               m_maxDistance = 5.0;
    float               m_minDistance = 0.0;
    rs2::threshold_filter m_thresholdFilter;

    int                 m_spatialMagnitude = 2;
    float               m_smoothAlpha = 0.5;
    int                 m_smoothDelta = 20;
    int                 m_spatialHoleFill = 0;
    rs2::spatial_filter m_spatialFilter;


    float               m_smoothAlphaTemporal = 0.4;
    int                 m_smoothDeltaTemporal = 20;
    int                 m_persistanceTemporal = 3;
    rs2::temporal_filter m_temporalFilter;

    int                 m_holeFill = 3;
    rs2::hole_filling_filter m_holeFilter;

public:
    rs2::frame apply(const rs2::frame & frame)
    {
        m_thresholdFilter.set_option(RS2_OPTION_MAX_DISTANCE, m_maxDistance);
        m_thresholdFilter.set_option(RS2_OPTION_MIN_DISTANCE, m_minDistance);
        rs2::frame temp = m_thresholdFilter.process(frame);

        if (m_decimation > 0)
        {
            m_decimationFilter.set_option(RS2_OPTION_FILTER_MAGNITUDE, m_decimation);
            temp = m_decimationFilter.process(temp);
        }

        if (m_spatialMagnitude > 0)
        {
            m_spatialFilter.set_option(RS2_OPTION_FILTER_MAGNITUDE, m_spatialMagnitude);
            m_spatialFilter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, m_smoothAlpha);
            m_spatialFilter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, m_smoothDelta);
            m_spatialFilter.set_option(RS2_OPTION_HOLES_FILL, m_spatialHoleFill);

            temp = m_spatialFilter.process(temp);
        }

        if (m_holeFill < 3)
        {
            m_holeFilter.set_option(RS2_OPTION_HOLES_FILL, m_holeFill);
            temp = m_holeFilter.process(temp);
        }

        if (m_smoothAlphaTemporal < 1.0f)
        {
            m_temporalFilter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, m_smoothAlphaTemporal);
            m_temporalFilter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, m_smoothDeltaTemporal);
            m_temporalFilter.set_option(RS2_OPTION_HOLES_FILL, m_persistanceTemporal);
            temp = m_temporalFilter.process(temp);
        }
        return temp;
    }

    void imgui()
    {

    }
};