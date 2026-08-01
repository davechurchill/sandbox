#pragma once

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API
#include "imgui.h"
#include "imgui-SFML.h"
#include "Profiler.hpp"
#include "Save.hpp"

#include <fstream>

struct CameraFilters
{
    float               m_smoothAlphaTemporal = 0.047f;
    int                 m_smoothDeltaTemporal = 72;
    int                 m_persistanceTemporal = 3;
    rs2::temporal_filter m_temporalFilter;

    int                 m_holeFill = 1;
    rs2::hole_filling_filter m_holeFilter;

public:
    rs2::frame apply(const rs2::frame & frame)
    {
        rs2::frame temp = frame;

        if (m_holeFill < 3)
        {
            PROFILE_SCOPE("Hole");
            m_holeFilter.set_option(RS2_OPTION_HOLES_FILL, (float)m_holeFill);
            temp = m_holeFilter.process(temp);
        }

        if (m_smoothAlphaTemporal < 1.0f)
        {
            PROFILE_SCOPE("Temporal");
            m_temporalFilter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, m_smoothAlphaTemporal);
            m_temporalFilter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, (float)m_smoothDeltaTemporal);
            m_temporalFilter.set_option(RS2_OPTION_HOLES_FILL, (float)m_persistanceTemporal);
            temp = m_temporalFilter.process(temp);
        }
        return temp;
    }

    void imgui()
    {
        PROFILE_FUNCTION();

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

    void save(Save & save) const
    {
        Save::Json & settings = save.section("CameraFilters");
        settings["m_smoothAlphaTemporal"] = m_smoothAlphaTemporal;
        settings["m_smoothDeltaTemporal"] = m_smoothDeltaTemporal;
        settings["m_persistanceTemporal"] = m_persistanceTemporal;
        settings["m_holeFill"] = m_holeFill;
    }

    void load(const Save & save)
    {
        const Save::Json & settings = save.section("CameraFilters");
        Save::read(settings, "m_smoothAlphaTemporal", m_smoothAlphaTemporal);
        Save::read(settings, "m_smoothDeltaTemporal", m_smoothDeltaTemporal);
        Save::read(settings, "m_persistanceTemporal", m_persistanceTemporal);
        Save::read(settings, "m_holeFill", m_holeFill);
    }
};
