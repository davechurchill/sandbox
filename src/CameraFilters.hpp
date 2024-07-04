#pragma once

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API
#include "imgui.h"
#include "imgui-SFML.h"

#include <fstream>

struct CameraFilters
{
    int                 m_decimation = 1;
    rs2::decimation_filter m_decimationFilter;

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
        rs2::frame temp = frame;

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
        ImGui::SliderInt("Decimation", &m_decimation, 0, 5);

        if (ImGui::CollapsingHeader("Spatial Filter"))
        {
            ImGui::Indent();
            ImGui::SliderInt("Magnitude", &m_spatialMagnitude, 0, 5);
            if (m_spatialMagnitude > 0)
            {
                ImGui::SliderFloat("Smooth Alpha", &m_smoothAlpha, 0.25, 1.0);
                ImGui::SliderInt("Smooth Delta", &m_smoothDelta, 1, 50);
                ImGui::SliderInt("Hole Filling", &m_spatialHoleFill, 0, 5);
            }
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Temporal Filter"))
        {
            ImGui::Indent();
            ImGui::SliderFloat("Temporal Alpha", &m_smoothAlphaTemporal, 0.0, 1.0);
            ImGui::SliderInt("Temporal Delta", &m_smoothDeltaTemporal, 1, 100);
            const char * persistance_options[] = { "Disabled", "Valid in 8/8", "Valid in 2/last 3", "Valid in 2/last 4", "Valid in 2/8",
                                                  "Valid in 1/last 2", "Valid in 1/last 5", "Valid in 1/last 8", "Persist Indefinitely" };
            ImGui::Combo("Persistance", &m_persistanceTemporal, persistance_options, 9);
            ImGui::Unindent();
        }

        if (ImGui::CollapsingHeader("Hole Filling Filter"))
        {
            ImGui::Indent();
            const char * hole_options[] = { "Fill from left", "Farthest from around", "Nearest from around", "Off" };
            ImGui::Combo("Fill Setting", &m_holeFill, hole_options, 4);
            ImGui::Unindent();
        }
    }

    void save(std::ofstream & fout)
    {
        fout << "Decimation" << " " << m_decimation << "\n";
        fout << "temporalAlpha" << " " << m_smoothAlphaTemporal << "\n";
        fout << "temporalDelta" << " " << m_smoothDeltaTemporal << "\n";
        fout << "temporalPersistance" << " " << m_persistanceTemporal << "\n";

        fout << "Magnitude" << " " << m_spatialMagnitude << "\n";
        fout << "Alpha" << " " << m_smoothAlpha << "\n";
        fout << "Delta" << " " << m_smoothDelta << "\n";
        fout << "SHole" << " " << m_spatialHoleFill << "\n";
        fout << "HoleFill" << " " << m_holeFill << "\n";
    }

    void loadTerm(const std::string & term, std::ifstream & fin)
    {
        if (term == "Decimation") { fin >> m_decimation; }
        if (term == "temporalAlpha") { fin >> m_smoothAlphaTemporal; }
        if (term == "temporalDelta") { fin >> m_smoothDeltaTemporal; }
        if (term == "temporalPersistance") { fin >> m_persistanceTemporal; }
        if (term == "Magnitude") { fin >> m_spatialMagnitude; }
        if (term == "Alpha") { fin >> m_smoothAlpha; }
        if (term == "SHole") { fin >> m_spatialHoleFill; }
        if (term == "HoleFill") { fin >> m_holeFill; }
    }
};