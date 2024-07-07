#include "Scene_Sandbox.h"
#include "Scene_Menu.h"
#include "GameEngine.h"
#include "Assets.h"
#include "Calibration.h"
#include "Profiler.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <string>

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

    m_contour.setContourLevel(0.5);

    loadConfig();

}

void Scene_Sandbox::captureImage()
{
    PROFILE_FUNCTION();

    rs2::frameset data;
    // Wait for next set of frames from the camera
    {
        PROFILE_SCOPE("rs2::wait_for_frames");
        data = m_pipe.wait_for_frames();
    }

    {
        PROFILE_SCOPE("rs2::alignment");
        if (m_alignment == alignment::depth) { data = m_alignment_depth.process(data); }
        else if (m_alignment == alignment::color) { data = m_alignment_color.process(data); }
    }

    // Handle regular video footage
    rs2::frame colorFrame;
    {
        PROFILE_SCOPE("rs2::get_color_frame");
        colorFrame = data.get_color_frame();
    }

    {
        PROFILE_SCOPE("Process Color Frame");
        const int cw = colorFrame.as<rs2::video_frame>().get_width();
        const int ch = colorFrame.as<rs2::video_frame>().get_height();
        if (m_drawColor)
        {
            m_cvColorImage = cv::Mat(cv::Size(cw, ch), CV_8UC3, (void*)colorFrame.get_data(), cv::Mat::AUTO_STEP);
            cv::cvtColor(m_cvColorImage, m_cvColorImage, cv::COLOR_RGB2RGBA);
            m_sfColorImage.create(m_cvColorImage.cols, m_cvColorImage.rows, m_cvColorImage.ptr());
            m_sfColorTexture.loadFromImage(m_sfColorImage);
            m_colorSprite.setTexture(m_sfColorTexture, true);
        }
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
        cv::Mat depthImage16u(cv::Size(dw, dh), CV_16U, (void*)depthFrame.get_data(), cv::Mat::AUTO_STEP);

        // convert the 16u image to a 32 bit floating point representation
        depthImage16u.convertTo(m_cvDepthImage32f, CV_32F);

        // multiply the image values by the unit type to get the data in meters like we want
        m_cvDepthImage32f = m_cvDepthImage32f * depthFrame.get_units();
    }

    dw = m_cvDepthImage32f.cols;
    dh = m_cvDepthImage32f.rows;

    {
        PROFILE_SCOPE("Copy Depth Data to Grid");
        // Copy data to depth grid
        if (m_depthGrid.width() != dw || m_depthGrid.height() != dh)
        {
            m_depthGrid.refill(dw, dh, 0.0);
        }
        if (m_maxDistance > m_minDistance)
        {
            for (int i = 0; i < dw; ++i)
            {
                for (int j = 0; j < dh; ++j)
                {
                    // Scale data to 0-1 range, where 1 is the highest point 
                    m_depthGrid.set(i, j, 1 - ((m_cvDepthImage32f.at<float>(j, i) - m_minDistance) / (m_maxDistance - m_minDistance)));
                }
            }
        }
    }

    // Calibration
    cv::Mat output;

    {
        PROFILE_SCOPE("Calibration TransformRect");
        m_calibration.transformRect(m_cvDepthImage32f, output);
    }


        //m_calibration.heightAdjustment(output);

    {
        PROFILE_SCOPE("Calibration TransformProjection");
        m_calibration.transformProjection(output, output);
    }


    // Draw warped depth image
    dw = output.cols;
    dh = output.rows;

    if (dw > 0 && dh > 0)
    {
        // Create warped data grid
        {
            PROFILE_SCOPE("Create Warped Data Grid");
            m_depthWarpedGrid.refill(dw, dh, 0.0f);
            if (m_maxDistance > m_minDistance)
            {
                for (int i = 0; i < dw; ++i)
                {
                    for (int j = 0; j < dh; ++j)
                    {
                        // Scale data to 0-1 range, where 1 is the highest point 
                        m_depthWarpedGrid.set(i, j, 1 - ((output.at<float>(j, i) - m_minDistance) / (m_maxDistance - m_minDistance)));
                    }
                }
            }
        }

        {
            PROFILE_SCOPE("Transformed Image Colorization");
            m_colorizer.color(m_transformedImage, m_depthWarpedGrid);
        }

        {
            PROFILE_SCOPE("Transformed Image to Texture");
            m_transformedTexture.loadFromImage(m_transformedImage);
            m_transformedSprite.setTexture(m_transformedTexture, true);
        }

        // Calculate Contour Lines from warped data grid
        if (m_drawContours)
        {
            PROFILE_SCOPE("Calculate Contour Lines");
            m_contour.init(dw, dh);
            m_contour.calculate(m_depthWarpedGrid);
        }
    }

    if (m_drawDepth)
    {
        {
            PROFILE_SCOPE("Depth Image Colorization");
            m_colorizer.color(m_sfDepthImage, m_depthGrid);
        }
        
        {
            PROFILE_SCOPE("Depth Image to Texture");
            m_sfDepthTexture.loadFromImage(m_sfDepthImage);
            m_depthSprite.setTexture(m_sfDepthTexture, true);
        }   
    }
}

void Scene_Sandbox::onFrame()
{
    if (m_cameraConnected)
    {
        captureImage();
    }
    else
    {
        attemptCameraConnection();
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
    ImGui::SFML::ProcessEvent(m_game->window(), event);
    m_viewController.processEvent(m_game->window(), event);

    // process the event within the calibration system
    // this will handle the moving of calibration quadrilaterals
    m_calibration.processEvent(event, m_mouseWorld);

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

    // happens whenever the mouse is being moved
    if (event.type == sf::Event::MouseMoved)
    {
        m_mouseScreen = { event.mouseMove.x, event.mouseMove.y };
        m_mouseWorld = m_game->window().mapPixelToCoords(m_mouseScreen);
    }
}

void Scene_Sandbox::sUserInput()
{
    sf::Event event;
    while (m_game->window().pollEvent(event))
    {
        sProcessEvent(event);
    }

    //sf::Event displayEvent;
    //while (m_game->displayWindow().pollEvent(displayEvent))
    //{
    //    sProcessEvent(displayEvent);
    //}
}

// renders the scene
void Scene_Sandbox::sRender()
{
    m_game->window().clear();
    m_game->displayWindow().clear();

    // draw the depth image
    if (m_drawDepth) { m_game->window().draw(m_depthSprite); }

    m_transformedSprite.setPosition(m_calibration.getTransformedPosition());
    float scale = m_calibration.getTransformedScale();
    m_transformedSprite.setScale(scale, scale);

    // draw the final transformed image in the chosen window
    if (m_game->displayWindow().isOpen()) { m_game->displayWindow().draw(m_transformedSprite); }
    else                                  { m_game->window().draw(m_transformedSprite); }

    // draw the contour lines 
    if (m_drawContours)
    {
        m_contourSprite.setTexture(m_contour.generateTexture(), true);
        m_contourSprite.setScale(scale, scale);
        m_contourSprite.setPosition(m_transformedSprite.getPosition());

        if (m_game->displayWindow().isOpen()) { m_game->displayWindow().draw(m_contourSprite); }
        else                                  { m_game->window().draw(m_contourSprite); }
    }

    // draw the color camera image 
    if (m_drawColor) { m_game->window().draw(m_colorSprite); }
    
    m_lineStrip.clear();
    m_quadArray.clear();
    m_game->window().draw(m_quadArray);
    m_game->window().draw(m_lineStrip);
    m_game->window().draw(m_text);

    // render the calibration debug information
    m_calibration.render(m_game->window());
}

void Scene_Sandbox::renderUI()
{
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

            m_colorizer.imgui();
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
            if (ImGui::InputInt("Contour Lines", &m_numberOfContourLines, 1, 10))
            {
                m_contour.setNumberofContourLines(m_numberOfContourLines);
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
            m_game->minecraft().imgui(m_depthGrid);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();

    }
    ImGui::End();
}

void Scene_Sandbox::attemptCameraConnection()
{
    rs2::context ctx;  // Create a context object, which is used to manage devices
    rs2::device_list devices = ctx.query_devices();  // Get a list of connected RealSense devices
    if (devices.size() > 0) // If at least one device is connected start pipe
    {
        m_pipe.start();
        m_cameraConnected = true;
    }
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

    saveConfig();
}