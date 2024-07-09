#include "Scene_Sandbox.h"
#include "Scene_Menu.h"
#include "GameEngine.h"
#include "Assets.h"
#include "Calibration.h"
#include "Profiler.hpp"
#include "RealSenseTools.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"


Scene_Sandbox::Scene_Sandbox(GameEngine * game)
    : Scene(game)
{
    init();
}

void Scene_Sandbox::init()
{
    ImGui::GetStyle().ScaleAllSizes(2.0f);
    ImGui::GetIO().FontGlobalScale = 2.0f;
            
    m_font = Assets::Instance().getFont("Tech");
    m_text.setFont(m_font);
    m_text.setPosition(10, 5);
    m_text.setCharacterSize(10);

    m_shaderPaths = 
    {
        "shaders/shader_popsicle.frag",
        "shaders/shader_red.frag",
        "shaders/shader_terrain.frag",
        ""
    };

    m_shader.loadFromFile(m_shaderPaths[0], sf::Shader::Fragment);

    loadConfig();
}

void Scene_Sandbox::connectToCamera()
{
    //RealSenseTools::PrintAvailableCameraModes();

    rs2::context ctx;  // Create a context object, which is used to manage devices
    rs2::device_list devices = ctx.query_devices();  // Get a list of connected RealSense devices
    if (devices.size() > 0) // If at least one device is connected start pipe
    {
        m_cameraConnected = true;

        int depthWidth = 1280, depthHeight = 720, depthFPS = 30; // depth camera hi res
        //int depthWidth = 848,  depthHeight = 480, depthFPS = 90; // depth camera hi fps
        int colorWidth = 1280, colorHeight = 720, colorFPS = 30; // color camera

        rs2::config cfg;
        cfg.enable_stream(RS2_STREAM_DEPTH, depthWidth, depthHeight, RS2_FORMAT_Z16,  depthFPS);  
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

sf::Image Scene_Sandbox::matToSfImage(const cv::Mat& mat) 
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
    image.create(rgb.cols, rgb.rows, rgb.ptr());

    return image;
}

void Scene_Sandbox::captureImages()
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
        m_cvColorImage = cv::Mat(cv::Size(cw, ch), CV_8UC3, (void*)colorFrame.get_data(), cv::Mat::AUTO_STEP);
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
        m_cvDepthImage16u = cv::Mat(cv::Size(dw, dh), CV_16U, (void*)depthFrame.get_data(), cv::Mat::AUTO_STEP);

        // convert the 16u image to a 32 bit floating point representation
        m_cvDepthImage16u.convertTo(m_cvDepthImage32f, CV_32F);

        // multiply the image values by the unit type to get the data in meters like we want
        m_depthFrameUnits = depthFrame.get_units();
        m_cvDepthImage32f = m_cvDepthImage32f * m_depthFrameUnits;
    }
        
    // set everything to 0 that's below min distance or above max distance
    // then scale the remaining values between min and max distance 0 to 1 (normalize)
    // store these values in a new 'normalized' cv::mat
    {
        PROFILE_SCOPE("Threshold and Normalize");
        cv::threshold(m_cvDepthImage32f, m_cvNormalizedDepthImage32f, m_minDistance, 255, cv::THRESH_TOZERO);
        cv::threshold(m_cvNormalizedDepthImage32f, m_cvNormalizedDepthImage32f, m_maxDistance, 255, cv::THRESH_TOZERO_INV);
        m_cvNormalizedDepthImage32f = (m_cvNormalizedDepthImage32f - m_minDistance) / (m_maxDistance - m_minDistance);
    }

    // Calibration
    {
        PROFILE_SCOPE("Calibration TransformRect");
        m_calibration.transformRect(m_cvNormalizedDepthImage32f, m_cvTransformedDepthImage32f);
        m_data = m_cvTransformedDepthImage32f;
    }

    //m_calibration.heightAdjustment(output);

    {
        PROFILE_SCOPE("Calibration TransformProjection");
        m_calibration.transformProjection(m_cvTransformedDepthImage32f, m_cvTransformedDepthImage32f);
    }
    
    // Draw warped depth image
    dw = m_cvTransformedDepthImage32f.cols;
    dh = m_cvTransformedDepthImage32f.rows;

    // if something went wrong above, quit the function
    if (dw == 0 || dh == 0) { return; }

    {
        int kernelSize = 17; // Example kernel size
        double sigmaX = 9.5; // Example standard deviation in X direction
        double sigmaY = 9.5; // Example standard deviation in Y direction

        cv::Mat blurredImage;
        {
            PROFILE_SCOPE("OpenCV Gaussian Blur");
            cv::GaussianBlur(m_cvTransformedDepthImage32f, blurredImage, cv::Size(kernelSize, kernelSize), sigmaX, sigmaY);
        }

        {
            PROFILE_SCOPE("Transformed Image SFML Image");
            m_sfTransformedDepthImage = matToSfImage(blurredImage);

            {
                PROFILE_SCOPE("SFML Texture From Image");
                m_sfTransformedDepthTexture.loadFromImage(m_sfTransformedDepthImage);
                m_sfTransformedDepthSprite.setTexture(m_sfTransformedDepthTexture, true);
            }
        }
    }

    if (m_drawDepth)
    {
        {
            PROFILE_SCOPE("Depth Image to SFML Image");
            sf::Image image = matToSfImage(m_cvNormalizedDepthImage32f);

            {
                PROFILE_SCOPE("SFML Texture From Image");
                m_sfDepthTexture.loadFromImage(image);
                m_depthSprite.setTexture(m_sfDepthTexture, true);
            }
        }   
    }
}

void Scene_Sandbox::onFrame()
{
    if (m_cameraConnected)
    {
        captureImages();
    }
    else
    {
        connectToCamera();
    }
    sUserInput();
    sRender();
    if (m_drawUI)
    {
        renderUI();
    }
    m_currentFrame++;
}

void Scene_Sandbox::sProcessEvent(const sf::Event& event)
{
    // this event triggers when the window is closed
    if (event.type == sf::Event::Closed)
    {
        endScene();
        m_game->quit();
    }

    // this event is triggered when a key is pressed
    if (event.type == sf::Event::KeyPressed)
    {
        switch (event.key.code)
        {
        
        case sf::Keyboard::Escape:
        {
            endScene();
            break;
        }

        case sf::Keyboard::I:
        {
            m_drawUI = !m_drawUI;
            break;
        }

        case sf::Keyboard::F:
        {
            if (!m_game->displayWindow().isOpen())
            {
                m_game->displayWindow().create(sf::VideoMode(1920, 1080), "Display", sf::Style::None);
                m_game->displayWindow().setPosition({ -1920, 0 });
            }
            else
            {
                m_game->displayWindow().close();
            }
        }
        }
    }

    if (event.type == sf::Event::MouseButtonPressed)
    {
        // happens when the left mouse button is pressed
        if (event.mouseButton.button == sf::Mouse::Left) { }
        if (event.mouseButton.button == sf::Mouse::Right) {}
    }

    // happens when the mouse button is released
    if (event.type == sf::Event::MouseButtonReleased)
    {
        if (event.mouseButton.button == sf::Mouse::Left) {}
        if (event.mouseButton.button == sf::Mouse::Right) {}
    }

}

void Scene_Sandbox::sUserInput()
{
    PROFILE_FUNCTION();

    sf::Event event;
    while (m_game->window().pollEvent(event))
    {
        ImGui::SFML::ProcessEvent(m_game->window(), event);
        m_viewController.processEvent(m_game->window(), event);
        sProcessEvent(event);

        // process the event within the calibration system
        // this will handle the moving of calibration quadrilaterals
        m_calibration.processDebugEvent(event, m_mouseWorld);
        if (!m_game->displayWindow().isOpen())
        {
            m_calibration.processDisplayEvent(event, m_mouseWorld);
        }

        // happens whenever the mouse is being moved
        if (event.type == sf::Event::MouseMoved)
        {
            m_mouseScreen = { event.mouseMove.x, event.mouseMove.y };
            m_mouseWorld = m_game->window().mapPixelToCoords(m_mouseScreen);
        }
    }

    sf::Event displayEvent;
    while (m_game->displayWindow().pollEvent(displayEvent))
    {
        sProcessEvent(displayEvent);

        // process the event within the calibration system
        // this will handle the moving of calibration quadrilaterals
        m_calibration.processDisplayEvent(displayEvent, m_mouseDisplay);

        // happens whenever the mouse is being moved
        if (displayEvent.type == sf::Event::MouseMoved)
        {
            m_mouseDisplay = { (float)displayEvent.mouseMove.x, (float)displayEvent.mouseMove.y };
        }
    }
}

// renders the scene
void Scene_Sandbox::sRender()
{
    PROFILE_FUNCTION();

    m_game->window().clear();
    m_game->displayWindow().clear();

    {
        PROFILE_SCOPE("Draw Depth Image");
        if (m_drawDepth) { m_game->window().draw(m_depthSprite); }
    }

    m_sfTransformedDepthSprite.setPosition(m_calibration.getTransformedPosition());
    float scale = m_calibration.getTransformedScale();
    m_sfTransformedDepthSprite.setScale(scale, scale);

    //m_shader.setUniform("texture", m_sfTransformedDepthSprite.getTexture());

    //Change color scheme
    
    m_shader.setUniform("contour", m_drawContours);
    m_shader.setUniform("numberOfContourLines", m_numberOfContourLines);

    {
        PROFILE_SCOPE("Draw Transformed Image");
        if (m_game->displayWindow().isOpen()) { m_game->displayWindow().draw(m_sfTransformedDepthSprite, &m_shader); }
        else { m_game->window().draw(m_sfTransformedDepthSprite, &m_shader); }
    }

    {
        PROFILE_SCOPE("Draw Color Image");
        if (m_drawColor) { m_game->window().draw(m_colorSprite); }
    }
    
    m_game->window().draw(m_text);

    // render the calibration debug information
    if (m_game->displayWindow().isOpen()) { m_calibration.render(m_game->window(), m_game->displayWindow()); }
    else { m_calibration.render(m_game->window(), m_game->window()); }
}

void Scene_Sandbox::renderUI()
{
    PROFILE_FUNCTION();

    ImGui::Begin("Options", &m_drawUI, ImGuiWindowFlags_MenuBar);

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::Button("Save")) { saveConfig(); }
        if (ImGui::Button("Load")) { loadConfig(); }
        ImGui::EndMenuBar();
    }

    if (!m_cameraConnected)
    {
        ImGui::Text("Please Connect Camera");
    }

    ImGui::Text("Framerate: %d", (int)m_game->framerate());

    if (ImGui::BeginTabBar("MyTabBar"))
    {
        if (ImGui::BeginTabItem("View"))
        {
            const char* items[] = { "Depth", "Color", "Nothing" };
            ImGui::Combo("Alignment", (int*)&m_alignment, items, 3);

            const char* shaders[] = { "Popsicle", "Red", "Terrain", "None"};
            if (ImGui::Combo("Color Scheme", &m_selectedShaderIndex, shaders, 4))
            {
                m_shader.loadFromFile(m_shaderPaths[m_selectedShaderIndex], sf::Shader::Fragment);
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

            ImGui::Spacing();

            ImGui::Checkbox("Draw Contour Lines", &m_drawContours);

            if (ImGui::Button("Screenshot Raw Depth"))
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
            m_filters.imgui();

            ImGui::EndTabItem();
        }
        

        if (ImGui::BeginTabItem("Calibration"))
        {
            m_calibration.imgui();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Minecraft"))
        {
            m_game->minecraft().imgui(m_data);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

    }
    ImGui::End();
}

void Scene_Sandbox::saveConfig()
{
    std::ofstream fout("config.txt");

    fout << "MaxDistance" << " " << m_maxDistance << "\n";
    fout << "MinDistance" << " " << m_minDistance << "\n";

    m_filters.save(fout);
    m_calibration.save(fout);
}

void Scene_Sandbox::loadConfig()
{
    std::ifstream fin("config.txt");
    if (!fin.good()) { return; }
    std::string temp;
    while (fin >> temp)
    {
        if (temp == "MaxDistance") { fin >> m_maxDistance; }
        if (temp == "MinDistance") { fin >> m_minDistance; }
        m_filters.loadTerm(temp, fin);
    }
    m_calibration.loadConfiguration();
}

void Scene_Sandbox::endScene()
{
    m_game->changeScene<Scene_Menu>("Menu");
    m_game->displayWindow().close();
    saveConfig();
}