#pragma once

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API

namespace RealSenseTools
{
    rs2::sensor FindSensor(rs2::device& device, rs2_stream stream_type) 
    {
        for (auto& sensor : device.query_sensors())
        {
            for (auto& profile : sensor.get_stream_profiles()) 
            {
                if (profile.stream_type() == stream_type) 
                {
                    return sensor;
                }
            }
        }
        throw std::runtime_error("Sensor not found for stream type");
    }

    struct Mode { int width = 0; int height = 0; int fps = 0; rs2_format format; };

    std::vector<Mode> GetAvailableVideoModes(rs2_stream stream_type)
    {
        try 
        {
            // Get a list of connected devices
            rs2::context ctx;
            rs2::device_list devices = ctx.query_devices();

            // Check if any device is connected
            if (devices.size() == 0) {
                std::cerr << "No device connected" << std::endl;
                return {};
            }

            // Get the sensor of the desired type
            rs2::device dev = devices[0];
            rs2::sensor sensor = FindSensor(dev, stream_type);

            std::vector<Mode> modes;

            // Print the available profiles for the depth sensor
            for (auto& profile : sensor.get_stream_profiles())
            {
                auto vp = profile.as<rs2::video_stream_profile>();
                modes.push_back({ vp.width(), vp.height(), vp.fps(), vp.format() });
            }

            return modes;
        }
        catch (const rs2::error& e) 
        {
            std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n" << e.what() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }

        return {};
    }

    void PrintAvailableCameraModes()
    {
        std::cout << "\n\nDepth Profiles:" << std::endl;
        auto depthModes = GetAvailableVideoModes(RS2_STREAM_DEPTH);
        for (auto& mode : depthModes)
        {
            printf("%6d%6d%6d%10s\n", mode.width, mode.height, mode.fps, rs2_format_to_string(mode.format));
        }

        std::cout << "\n\nColor Profiles:" << std::endl;
        auto colorModes = GetAvailableVideoModes(RS2_STREAM_COLOR);
        for (auto& mode : colorModes)
        {
            printf("%6d%6d%6d%10s\n", mode.width, mode.height, mode.fps, rs2_format_to_string(mode.format));
        }
    }
}