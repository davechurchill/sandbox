#pragma once

#include "Tools.h"
#include "Profiler.hpp"
#include "imgui.h"

#include <format>
#include <limits>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace Tools
{
    namespace
    {
    struct DisplayTarget
    {
        sf::VideoMode mode;
        sf::Vector2i position;
    };

    struct MonitorOption
    {
        std::string id;
        std::string label;
    };

#if defined(_WIN32)
    BOOL CALLBACK collectMonitor(
        HMONITOR monitor,
        HDC,
        LPRECT,
        LPARAM monitorListAddress)
    {
        auto & monitors = *reinterpret_cast<std::vector<HMONITOR> *>(monitorListAddress);
        monitors.push_back(monitor);
        return TRUE;
    }

    std::vector<HMONITOR> getMonitors()
    {
        std::vector<HMONITOR> monitors;
        if (!EnumDisplayMonitors(
            nullptr,
            nullptr,
            collectMonitor,
            reinterpret_cast<LPARAM>(&monitors)))
        {
            monitors.clear();
        }
        return monitors;
    }

    bool getMonitorInfo(HMONITOR monitor, MONITORINFOEXA & info)
    {
        info = {};
        info.cbSize = sizeof(info);
        return GetMonitorInfoA(monitor, &info) != FALSE;
    }

    DisplayTarget makeDisplayTarget(const MONITORINFOEXA & info)
    {
        const LONG width = info.rcMonitor.right - info.rcMonitor.left;
        const LONG height = info.rcMonitor.bottom - info.rcMonitor.top;
        return {
            sf::VideoMode({ (unsigned int)width, (unsigned int)height }),
            { info.rcMonitor.left, info.rcMonitor.top }
        };
    }
#endif

    std::vector<MonitorOption> getMonitorOptions(const sf::Window & mainWindow)
    {
#if defined(_WIN32)
        std::vector<MonitorOption> options;
        const HMONITOR mainMonitor = MonitorFromWindow(
            mainWindow.getNativeHandle(),
            MONITOR_DEFAULTTOPRIMARY);

        for (HMONITOR monitor : getMonitors())
        {
            MONITORINFOEXA info{};
            if (!getMonitorInfo(monitor, info))
            {
                continue;
            }

            std::string displayName = info.szDevice;
            if (displayName.starts_with("\\\\.\\"))
            {
                displayName.erase(0, 4);
            }

            const bool isMain = monitor == mainMonitor;
            const bool isPrimary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
            std::string role;
            if (isMain && isPrimary) { role = " [Main, Primary]"; }
            else if (isMain) { role = " [Main]"; }
            else if (isPrimary) { role = " [Primary]"; }

            const LONG width = info.rcMonitor.right - info.rcMonitor.left;
            const LONG height = info.rcMonitor.bottom - info.rcMonitor.top;
            options.push_back({
                info.szDevice,
                std::format(
                    "{} - {}x{} at ({}, {}){}",
                    displayName,
                    width,
                    height,
                    info.rcMonitor.left,
                    info.rcMonitor.top,
                    role)
            });
        }
        return options;
#else
        const sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        return {{
            "Desktop",
            std::format("Desktop - {}x{}", desktop.size.x, desktop.size.y)
        }};
#endif
    }

    DisplayTarget getDisplayTarget(
        const sf::Window & mainWindow,
        const std::string & selectedMonitorID)
    {
#if defined(_WIN32)
        DisplayTarget target{ sf::VideoMode::getDesktopMode(), { 0, 0 } };
        const HMONITOR mainMonitor = MonitorFromWindow(
            mainWindow.getNativeHandle(),
            MONITOR_DEFAULTTOPRIMARY);

        const std::vector<HMONITOR> monitors = getMonitors();
        if (!selectedMonitorID.empty())
        {
            for (HMONITOR monitor : monitors)
            {
                MONITORINFOEXA info{};
                if (getMonitorInfo(monitor, info)
                    && selectedMonitorID == info.szDevice)
                {
                    return makeDisplayTarget(info);
                }
            }
        }

        MONITORINFOEXA mainInfo{};
        if (!mainMonitor || !getMonitorInfo(mainMonitor, mainInfo))
        {
            return target;
        }

        HMONITOR selectedMonitor = mainMonitor;
        MONITORINFOEXA selectedInfo = mainInfo;
        long long closestDistance = std::numeric_limits<long long>::max();
        const long long mainCenterX = (long long)mainInfo.rcMonitor.left
            + mainInfo.rcMonitor.right;
        const long long mainCenterY = (long long)mainInfo.rcMonitor.top
            + mainInfo.rcMonitor.bottom;

        for (HMONITOR monitor : monitors)
        {
            if (monitor == mainMonitor)
            {
                continue;
            }

            MONITORINFOEXA info{};
            if (!getMonitorInfo(monitor, info))
            {
                continue;
            }

            const long long deltaX = (long long)info.rcMonitor.left
                + info.rcMonitor.right - mainCenterX;
            const long long deltaY = (long long)info.rcMonitor.top
                + info.rcMonitor.bottom - mainCenterY;
            const long long distance = deltaX * deltaX + deltaY * deltaY;
            if (distance < closestDistance)
            {
                closestDistance = distance;
                selectedMonitor = monitor;
                selectedInfo = info;
            }
        }

        return selectedMonitor ? makeDisplayTarget(selectedInfo) : target;
#else
        return { sf::VideoMode::getDesktopMode(), { 0, 0 } };
#endif
    }
    }

    bool imguiMonitorSelector(
        const sf::Window & mainWindow,
        std::string & selectedMonitorID)
    {
        const std::vector<MonitorOption> monitorOptions = getMonitorOptions(mainWindow);
        std::string monitorPreview = "Automatic (nearest other monitor)";
        bool selectedMonitorAvailable = selectedMonitorID.empty();
        for (const MonitorOption & option : monitorOptions)
        {
            if (option.id == selectedMonitorID)
            {
                monitorPreview = option.label;
                selectedMonitorAvailable = true;
                break;
            }
        }
        if (!selectedMonitorAvailable)
        {
            monitorPreview = "Unavailable monitor (using Automatic)";
        }

        bool changed = false;
        if (ImGui::BeginCombo("Display Monitor", monitorPreview.c_str()))
        {
            if (ImGui::Selectable(
                "Automatic (nearest other monitor)",
                selectedMonitorID.empty()))
            {
                changed = !selectedMonitorID.empty();
                selectedMonitorID.clear();
            }
            for (const MonitorOption & option : monitorOptions)
            {
                if (ImGui::Selectable(
                    option.label.c_str(),
                    option.id == selectedMonitorID))
                {
                    changed = option.id != selectedMonitorID;
                    selectedMonitorID = option.id;
                }
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    void openDisplayWindow(
        sf::RenderWindow & displayWindow,
        const sf::Window & mainWindow,
        const std::string & selectedMonitorID)
    {
        const DisplayTarget target = getDisplayTarget(mainWindow, selectedMonitorID);
        displayWindow.create(target.mode, "Display", sf::Style::None);
        displayWindow.setPosition(target.position);
    }

    // given an (mx, my) mouse position, return the index of the first circle the contains the position
    // returns -1 if the mouse position is not inside any circle
    int getClickedCircleIndex(float mx, float my, std::vector<sf::CircleShape> & circles)
    {
        PROFILE_FUNCTION();
        for (int i = 0; i < circles.size(); i++)
        {
            float dx = mx - circles[i].getPosition().x;
            float dy = my - circles[i].getPosition().y;
            float d2 = dx * dx + dy * dy;
            float radiusSquared = circles[i].getRadius() * circles[i].getRadius();
            if (d2 <= radiusSquared) { return i; }
        }

        return -1;
    }

    sf::Image matToSfImage(const cv::Mat & mat)
    {
        PROFILE_FUNCTION();

        // Ensure the input image is in the correct format (CV_32F)
        cv::Mat normalized;
        mat.convertTo(normalized, CV_8U, 255.0); // Scale float [0, 1] to [0, 255]

        // Convert to RGB (SFML requires RGB format)
        cv::Mat rgb;
        cv::cvtColor(normalized, rgb, cv::COLOR_GRAY2RGBA);

        // Create SFML image
        sf::Image image;
        image.resize({ (unsigned int)rgb.cols, (unsigned int)rgb.rows }, rgb.ptr());

        return image;
    }

}
