#include "Source_Camera.h"
#include "Tools.h"
#include "Profiler.hpp"

void Source_Camera::init()
{
}

void Source_Camera::connectToCamera()
{
    //RealSenseTools::PrintAvailableCameraModes();

    rs2::context ctx;  // Create a context object, which is used to manage devices
    rs2::device_list devices = ctx.query_devices();  // Get a list of connected RealSense devices
    if (devices.size() > 0) // If at least one device is connected start pipe
    {
        m_cameraConnected = true;

        int depthWidth = 1280, depthHeight = 720, depthFPS = 30;
        switch (m_fpsSetting) {
        case 0: depthWidth = 1280, depthHeight = 720, depthFPS = 30; break;
        case 1: depthWidth = 848, depthHeight = 480, depthFPS = 90; break;
        }

        int colorWidth = 1280, colorHeight = 720, colorFPS = 30; // color camera

        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_DEPTH, depthWidth, depthHeight, RS2_FORMAT_Z16, depthFPS);
        cfg.enable_stream(RS2_STREAM_COLOR, colorWidth, colorHeight, RS2_FORMAT_RGB8, colorFPS);

        // start the rs2 pipe and get the profile
        rs2::pipeline_profile profile = m_pipe.start(cfg);

        // Extract the video stream profile
        auto depthStreamProfile = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
        auto colorStreamProfile = profile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();

        // Print the resolution and frame rate
        std::cout << "\nDepth Camera: " << depthStreamProfile.width() << " x " << depthStreamProfile.height() << " @ " << depthStreamProfile.fps() << " FPS\n";
        std::cout << "\nColor Camera: " << colorStreamProfile.width() << " x " << colorStreamProfile.height() << " @ " << colorStreamProfile.fps() << " FPS\n";
    }
}

void Source_Camera::captureImages()
{
    PROFILE_FUNCTION();

    // Wait for next set of frames from the camera
    rs2::frameset data;
    {
        PROFILE_SCOPE("rs2::wait_for_frames");
        data = m_pipe.wait_for_frames();
    }

    // align the color and depth images if we have chosen to
    {
        PROFILE_SCOPE("rs2::alignment");
        if (m_alignment == alignment::depth) { data = m_alignment_depth.process(data); }
        else if (m_alignment == alignment::color) { data = m_alignment_color.process(data); }
    }

    // capture the color image
    if (m_drawColor)
    {
        rs2::frame colorFrame;
        {
            PROFILE_SCOPE("rs2::get_color_frame");
            colorFrame = data.get_color_frame();
        }

        PROFILE_SCOPE("Process Color Frame");
        const int cw = colorFrame.as<rs2::video_frame>().get_width();
        const int ch = colorFrame.as<rs2::video_frame>().get_height();
        m_cvColorImage = cv::Mat(cv::Size(cw, ch), CV_8UC3, (void *)colorFrame.get_data(), cv::Mat::AUTO_STEP);
        cv::cvtColor(m_cvColorImage, m_cvColorImage, cv::COLOR_RGB2RGBA);
        m_sfColorImage.create(m_cvColorImage.cols, m_cvColorImage.rows, m_cvColorImage.ptr());
        m_sfColorTexture.loadFromImage(m_sfColorImage);
        m_colorSprite.setTexture(m_sfColorTexture, true);
    }

    // Handle depth feed
    rs2::depth_frame depthFrame = data.get_depth_frame();
    {
        PROFILE_SCOPE("Apply Depth Filters");
        depthFrame = m_filters.apply(depthFrame);
    }

    // Query frame size (width and height)
    int dw = depthFrame.as<rs2::video_frame>().get_width();
    int dh = depthFrame.as<rs2::video_frame>().get_height();

    {
        PROFILE_SCOPE("Make OpenCV from Depth");
        // create an opencv image from the raw depth frame data, which is 16-bit unsigned int
        m_cvDepthImage16u = cv::Mat(cv::Size(dw, dh), CV_16U, (void *)depthFrame.get_data(), cv::Mat::AUTO_STEP);

        // convert the 16u image to a 32 bit floating point representation
        m_cvDepthImage16u.convertTo(m_cvDepthImage32f, CV_32F);

        // multiply the image values by the unit type to get the data in meters like we want
        m_depthFrameUnits = depthFrame.get_units();
        m_cvDepthImage32f = m_cvDepthImage32f * m_depthFrameUnits;
    }

    // Perform height adjustment if turned on
    if (m_warper.shouldAdjustHeight())
    {
        PROFILE_SCOPE("Height Adjustment");
        m_warper.heightAdjustment(m_cvDepthImage32f);
    }

    // Perform Gaussian Blur of data if turned on
    // Note: Guassian Blur must be applied before thresholding or else the zeros created by the threshold will blur with the real values.
    //       It must also be applied before the transformation, or else the values in the matrix that represent black space will blur with the image.
    if (m_gaussianBlur)
    {
        PROFILE_SCOPE("OpenCV Gaussian Blur");

        int kernelSize = 17; // Example kernel size
        double sigmaX = 9.5; // Example standard deviation in X direction
        double sigmaY = 9.5; // Example standard deviation in Y direction
        cv::GaussianBlur(m_cvDepthImage32f, m_cvDepthImage32f, cv::Size(kernelSize, kernelSize), sigmaX, sigmaY);
    }

    // set everything to 0 that's below min distance or above max distance
    // then scale the remaining values between min and max distance 0 to 1 (normalize)
    // store these values in a new 'normalized' cv::mat
    {
        PROFILE_SCOPE("Threshold and Normalize");
        cv::threshold(m_cvDepthImage32f, m_cvNormalizedDepthImage32f, m_minDistance, 255, cv::THRESH_TOZERO);
        cv::threshold(m_cvNormalizedDepthImage32f, m_cvNormalizedDepthImage32f, m_maxDistance, 255, cv::THRESH_TOZERO_INV);
        m_cvNormalizedDepthImage32f = 1.f - (m_cvNormalizedDepthImage32f - m_minDistance) / (m_maxDistance - m_minDistance);
        cv::threshold(m_cvNormalizedDepthImage32f, m_cvNormalizedDepthImage32f, 0.99, 255, cv::THRESH_TOZERO_INV);
    }

    if (m_drawDepth)
    {
        {
            PROFILE_SCOPE("Depth Image to SFML Image");
            m_sfDepthImage = Tools::matToSfImage(m_cvNormalizedDepthImage32f);

            {
                PROFILE_SCOPE("SFML Texture From Image");
                m_sfDepthTexture.loadFromImage(m_sfDepthImage);
                m_depthSprite.setTexture(m_sfDepthTexture, true);
            }
        }
    }

    // Calibration
    {
        PROFILE_SCOPE("Calibration TransformRect");
        m_warper.transformRect(m_cvNormalizedDepthImage32f, m_data);
    }
}

void Source_Camera::imgui()
{
    PROFILE_FUNCTION();
    if (ImGui::BeginTabBar("CameraTabBar"))
    {
        if (ImGui::BeginTabItem("View"))
        {

            const char * items[] = { "Depth", "Color", "Nothing" };
            ImGui::Combo("Alignment", (int *)&m_alignment, items, IM_ARRAYSIZE(items));

            const char* settings[] = {"1280w 720h 30fps", "848w 480h 90fps"};
            if (ImGui::Combo("FPS / Resolution", &m_fpsSetting, settings, IM_ARRAYSIZE(settings))) {
                m_pipe.stop();
                connectToCamera();
            }

            if (ImGui::CollapsingHeader("Thresholds"))
            {
                ImGui::Indent();

                ImGui::SliderFloat("Max Distance", &m_maxDistance, 0.0, 2.0);
                ImGui::SliderFloat("Min Distance", &m_minDistance, 0.0, 2.0);
                ImGui::Unindent();
            }

            ImGui::Checkbox("Depth", &m_drawDepth);

            ImGui::Checkbox("Color", &m_drawColor);

            if (ImGui::Button("Screenshot Raw Depth Data"))
            {
                cv::Mat depthImage8u;
                m_cvDepthImage32f.convertTo(depthImage8u, CV_8U, 255.0 / m_depthFrameUnits);
                cv::normalize(m_cvDepthImage32f, depthImage8u, 0, 255, cv::NORM_MINMAX, CV_8U);
                cv::imwrite("depthImage_raw.png", depthImage8u);
            }
            if (ImGui::Button("Screenshot Normalized Depth"))
            {
                cv::Mat depthImage8u;
                m_cvNormalizedDepthImage32f.convertTo(depthImage8u, CV_8U, 255.0);
                cv::normalize(m_cvNormalizedDepthImage32f, depthImage8u, 0, 255, cv::NORM_MINMAX, CV_8U);
                cv::imwrite("depthImage_normalized.png", depthImage8u);
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Filters"))
        {
            ImGui::Checkbox("Gaussian Blur", &m_gaussianBlur);
            m_filters.imgui();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Calibration"))
        {
            m_warper.imgui();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void Source_Camera::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();
    {
        PROFILE_SCOPE("Draw Depth Image");
        if (m_drawDepth) { window.draw(m_depthSprite); }
    }

    {
        PROFILE_SCOPE("Draw Color Image");
        if (m_drawColor) { window.draw(m_colorSprite); }
    }

    m_warper.render(window);
}

void Source_Camera::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    m_warper.processEvent(event, mouse);
}

void Source_Camera::save(Save & save) const
{
    save.align = (int)m_alignment;
    save.gaussianBlur = m_gaussianBlur;
    save.maxDistance = m_maxDistance;
    save.minDistance = m_minDistance;
    save.drawColor = m_drawColor;
    save.drawDepth = m_drawDepth;
    save.fpsSetting = m_fpsSetting;
    m_filters.save(save);
    m_warper.save(save);
}
void Source_Camera::load(const Save & save)
{
    m_alignment = static_cast<alignment>(save.align);
    m_gaussianBlur = save.gaussianBlur;
    m_maxDistance = save.maxDistance;
    m_minDistance = save.minDistance;
    m_drawColor = save.drawColor;
    m_drawDepth = save.drawDepth;
    m_fpsSetting = save.fpsSetting;
    m_filters.load(save);
    m_warper.load(save);
}

cv::Mat Source_Camera::getTopography()
{
    if (!m_cameraConnected)
    {
        connectToCamera();
        return cv::Mat();
    } 

    captureImages();
    return m_data;
}