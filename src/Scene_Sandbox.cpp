#include "Scene_Sandbox.h"
#include "Scene_Menu.h"
#include "GameEngine.h"
#include "Assets.h"
#include "Calibration.h"

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
}

void Scene_Sandbox::captureImage()
{
    // Wait for next set of frames from the camera
    rs2::frameset data = m_pipe.wait_for_frames();

    if (m_alignment == alignment::depth)
    {
        data = m_alignment_depth.process(data);
    }
    else if (m_alignment == alignment::color)
    {
        data = m_alignment_color.process(data);
    }

    m_thresholdFilter.set_option(RS2_OPTION_MAX_DISTANCE, m_maxDistance);
    m_thresholdFilter.set_option(RS2_OPTION_MIN_DISTANCE, m_minDistance);
    data = m_thresholdFilter.process(data);

    // Handle regular video footage
    rs2::frame color = data.get_color_frame();
    const int cw = color.as<rs2::video_frame>().get_width();
    const int ch = color.as<rs2::video_frame>().get_height();
    if (m_drawColor)
    {
        m_cvColorImage = cv::Mat(cv::Size(cw, ch), CV_8UC3, (void *)color.get_data(), cv::Mat::AUTO_STEP);
        cv::cvtColor(m_cvColorImage, m_cvColorImage, cv::COLOR_RGB2RGBA);
        m_calibration.transform(m_cvColorImage);
        m_sfColorImage.create(m_cvColorImage.cols, m_cvColorImage.rows, m_cvColorImage.ptr());
        m_sfColorTexture.loadFromImage(m_sfColorImage);
        m_colorSprite.setTexture(m_sfColorTexture);
    }


    // Handle depth feed
    rs2::depth_frame rawDepth = data.get_depth_frame();

    if (m_decimation > 0)
    {
        m_decimationFilter.set_option(RS2_OPTION_FILTER_MAGNITUDE, m_decimation);
        rawDepth = m_decimationFilter.process(rawDepth);
    }

    if (m_spatialMagnitude > 0)
    {
        m_spatialFilter.set_option(RS2_OPTION_FILTER_MAGNITUDE, m_spatialMagnitude);
        m_spatialFilter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, m_smoothAlpha);
        m_spatialFilter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, m_smoothDelta);
        m_spatialFilter.set_option(RS2_OPTION_HOLES_FILL, m_spatialHoleFill);

        rawDepth = m_spatialFilter.process(rawDepth);
    }

    if (m_holeFill < 3)
    {
        m_holeFilter.set_option(RS2_OPTION_HOLES_FILL, m_holeFill);
        rawDepth = m_holeFilter.process(rawDepth);
    }

    if (m_smoothAlphaTemporal < 1.0f)
    {
        m_temporalFilter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, m_smoothAlphaTemporal);
        rawDepth = m_temporalFilter.process(rawDepth);
    }

    // Query frame size (width and height)
    int dw = rawDepth.as<rs2::video_frame>().get_width();
    int dh = rawDepth.as<rs2::video_frame>().get_height();

    // Save depth at current mouse location
    Vec2 pos;
    pos.x = m_mouseWorld.x / m_depthSprite.getGlobalBounds().width;
    pos.y = m_mouseWorld.y / m_depthSprite.getGlobalBounds().height;
    if (pos.x < 1 && pos.y < 1 && pos.x >= 0 && pos.y >= 0)
    {
        pos.x *= dw;
        pos.y *= dh;
        m_mouseDepth = rawDepth.get_distance(pos.x, pos.y);
    }
    else
    {
        m_mouseDepth = -1.0;
    }

    // Make OpenCV matrix from raw depth data
    m_cvRawDepthImage.create(cv::Size(dw, dh), CV_32F);
    for (int i = 0; i < dw; ++i)
    {
        for (int j = 0; j < dh; ++j)
        {
            m_cvRawDepthImage.at<float>(j, i) = rawDepth.get_distance(i, j);
        }
    }

    // Calibration
    m_calibration.transform(m_cvRawDepthImage);
    dw = m_cvRawDepthImage.cols;
    dh = m_cvRawDepthImage.rows;

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
                m_depthGrid.set(i, j, 1 - ((m_cvRawDepthImage.at<float>(j,i) - m_minDistance) / (m_maxDistance - m_minDistance)));
            }
        }
    }

    // Calculate Contour Lines
    if (m_drawContours)
    {
        m_contour.init(dw, dh);
        m_contour.calculate(m_depthGrid);
    }

    if (m_drawDepth)
    {
        // Create colorized depth image, scaling to the size of the video so they can align
        //cv::resize(m_cvRawDepthImage, m_cvRawDepthImage, cv::Size(cw, ch), (0, 0), (0, 0), cv::InterpolationFlags::INTER_NEAREST);

        m_sfDepthImage.create(dw, dh);
        for (int i = 0; i < dw; ++i)
        {
            for (int j = 0; j < dh; ++j)
            {
                float height = (m_cvRawDepthImage.at<float>(j, i) - m_minDistance) / (m_maxDistance - m_minDistance);
                if (height < 0.0f)
                {
                    m_sfDepthImage.setPixel(i, j, sf::Color::Black);
                    continue;
                }
                
                m_sfDepthImage.setPixel(i, j, colorize(height));
            }
        }

        // Create the SFML sprites to be rendered

        m_sfDepthTexture.loadFromImage(m_sfDepthImage);
        m_depthSprite.setTexture(m_sfDepthTexture, true);
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

void Scene_Sandbox::sUserInput()
{
    sf::Event event;
    while (m_game->window().pollEvent(event))
    {
        ImGui::SFML::ProcessEvent(m_game->window(), event);
        m_viewController.processEvent(m_game->window(), event);
        m_calibration.processEvent(event, m_mouseWorld);

        // this event triggers when the window is closed
        if (event.type == sf::Event::Closed)
        {
            m_game->quit();
        }

        // this event is triggered when a key is pressed
        if (event.type == sf::Event::KeyPressed)
        {
            switch (event.key.code)
            {
                case sf::Keyboard::Escape:
                {
                    m_game->changeScene<Scene_Menu>("Menu");
                    break;
                }

                case sf::Keyboard::I:
                {
                    m_drawUI = !m_drawUI;
                    break;
                }
            }
        }

        if (event.type == sf::Event::MouseButtonPressed)
        {
            // happens when the left mouse button is pressed
            if (event.mouseButton.button == sf::Mouse::Left) 
            {
                switch (m_mouseSelection)
                {
                case MouseSelections::MaxDistance: 
                    {
                        if (m_mouseDepth >= 0.0)
                        {
                            m_maxDistance = m_mouseDepth;
                        }
                    } break;
                case MouseSelections::MinDistance: 
                    {
                        if (m_mouseDepth >= 0.0)
                        {
                            m_minDistance = m_mouseDepth;
                        }
                    } break;
                }
                m_mouseSelection = MouseSelections::None;
            }
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
}

// renders the scene
void Scene_Sandbox::sRender()
{
    const sf::Color gridColor(64, 64, 64);

    m_game->window().clear();
    m_lineStrip.clear();
    m_quadArray.clear();

    m_depthSprite.setColor(sf::Color(255, 255, 255, m_depthAlpha));
    m_depthSprite.setPosition(m_depthPos[0], m_depthPos[1]);
    m_depthSprite.setScale(m_depthScaleX, m_depthScaleY);

    m_colorSprite.setColor(sf::Color(255, 255, 255, m_colorAlpha));
    m_colorSprite.setPosition(m_colorPos[0], m_colorPos[1]);
    m_colorSprite.setScale(m_colorScale, m_colorScale);

    if (m_drawDepth) { m_game->window().draw(m_depthSprite); }
    if (m_drawColor) { m_game->window().draw(m_colorSprite); }
    
    m_game->window().draw(m_quadArray);
    m_game->window().draw(m_lineStrip);
    m_game->window().draw(m_text);

    if (m_drawContours)
    {
        m_contourSprite.setTexture(m_contour.generateTexture(), true);
        float scaleX = m_depthScaleX * (float)m_sfDepthImage.getSize().x / m_depthGrid.width();
        float scaleY = m_depthScaleY * (float)m_sfDepthImage.getSize().y / m_depthGrid.height();
        m_contourSprite.setScale(scaleX, scaleY);
        m_game->window().draw(m_contourSprite);
    }

    m_calibration.render(m_game->window());
}

void Scene_Sandbox::renderUI()
{
    const char vals[7] = { '.', 'G', '@', 'O', 'T', 'S', 'W' };
   
    ImGui::Begin("Options");

    if (!m_cameraConnected)
    {
        ImGui::Text("Please Connect Camera");
    }

    ImGui::Text("Framerate: %d", (int)m_game->framerate());

    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        saveConfig();
    }
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        loadConfig();
    }

    if (ImGui::BeginTabBar("MyTabBar"))
    {
        if (ImGui::BeginTabItem("Camera"))
        {
            // PC Display Options
            const char * items[] = { "Depth", "Color", "Nothing" };
            ImGui::Combo("Alignment", (int *)&m_alignment, items, 3);

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
                ImGui::SliderFloat("Filter Smooth Alpha", &m_smoothAlphaTemporal, 0.0, 1.0);
                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Threshold Filter"))
            {
                ImGui::Indent();
                ImGui::Text("Distance from Mouse %f", m_mouseDepth);

                ImGui::SliderFloat("Max Distance", &m_maxDistance, 0.0, 7.0);
                if (ImGui::Button("Select Max Distance"))
                {
                    m_mouseSelection = MouseSelections::MaxDistance;
                }

                ImGui::SliderFloat("Min Distance", &m_minDistance, 0.0, 7.0);
                if (ImGui::Button("Select Min Distance"))
                {
                    m_mouseSelection = MouseSelections::MinDistance;
                }
                ImGui::Unindent();
            }

            if (ImGui::CollapsingHeader("Hole Filling Filter"))
            {
                ImGui::Indent();
                const char* hole_options[] = { "Fill from left", "Farthest from around", "Nearest from around", "Off"};
                ImGui::Combo("Fill Setting", &m_holeFill, hole_options, 4);
                ImGui::Unindent();
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("View"))
        {
            ImGui::Checkbox("Depth", &m_drawDepth);
            if (ImGui::Button("Match Color"))
            {
                m_depthAlpha = m_colorAlpha;
                m_depthPos[0] = m_colorPos[0];
                m_depthPos[1] = m_colorPos[1];
                m_depthScaleX = m_colorScale;
                m_depthScaleY = m_colorScale;
            }
            ImGui::SliderInt("DAlpha", &m_depthAlpha, 0, 255);
            ImGui::SliderFloat2("DPos", m_depthPos, -1000, 1000);
            ImGui::SliderFloat("DScale X", &m_depthScaleX, 0, 5);
            ImGui::SliderFloat("DScale Y", &m_depthScaleY, 0, 5);

            ImGui::Checkbox("Color", &m_drawColor);
            if (ImGui::Button("Match Depth"))
            {
                m_colorAlpha = m_depthAlpha;
                m_colorPos[0] = m_depthPos[0];
                m_colorPos[1] = m_depthPos[1];
                m_colorScale = m_depthScaleX;
            }
            ImGui::SliderInt("CAlpha", &m_colorAlpha, 0, 255);
            ImGui::SliderFloat2("CPos", m_colorPos, -1000, 1000);
            ImGui::SliderFloat("CScale", &m_colorScale, 0, 2);

            ImGui::Spacing();

            ImGui::Checkbox("Draw Contour Lines", &m_drawContours);
            if (ImGui::InputInt("Contour Lines", &m_numberOfContourLines, 1, 10))
            {
                m_contour.setNumberofContourLines(m_numberOfContourLines);
            }
            if (ImGui::Button("Toggle Fullscreen"))
            {
                m_game->toggleFullscreen();
            }

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
    fout << "Decimation" << " " << m_decimation << "\n";
    fout << "temporalAlpha" << " " << m_smoothAlphaTemporal << "\n";
    fout << "Magnitude" << " " << m_spatialMagnitude << "\n";
    fout << "Alpha" << " " << m_smoothAlpha << "\n";
    fout << "Delta" << " " << m_smoothDelta << "\n";
    fout << "SHole" << " " << m_spatialHoleFill << "\n";
    fout << "MaxDistance" << " " << m_maxDistance << "\n";
    fout << "MinDistance" << " " << m_minDistance << "\n";
    fout << "HoleFill" << " " << m_holeFill << "\n";

    fout << "ViewCenter" << " " << m_game->window().getView().getCenter().x << " "
        << m_game->window().getView().getCenter().y << "\n";

    fout << "ViewSize" << " " << m_game->window().getView().getSize().x << " "
        << m_game->window().getView().getSize().y << "\n";

    fout << "m_points[0]" << " " << m_calibration.getConfig()[0].x << " " << m_calibration.getConfig()[0].y << "\n";
    fout << "m_points[1]" << " " << m_calibration.getConfig()[1].x << " " << m_calibration.getConfig()[1].y << "\n";
    fout << "m_points[2]" << " " << m_calibration.getConfig()[2].x << " " << m_calibration.getConfig()[2].y << "\n";
    fout << "m_points[3]" << " " << m_calibration.getConfig()[3].x << " " << m_calibration.getConfig()[3].y << "\n";

    fout << "m_width" << " " << m_calibration.getDimension().x << "\n";
    fout << "m_height" << " " << m_calibration.getDimension().y << "\n";
    fout << "m_drawContours" << " " << m_drawContours << "\n";
    fout << "m_numberOfContourLines" << " " << m_numberOfContourLines << "\n";
}

void Scene_Sandbox::loadConfig()
{
    std::ifstream fin("config.txt");
    std::string temp;
    while (fin >> temp)
    {
        if (temp == "Decimation") { fin >> m_decimation; }
        if (temp == "temporalAlpha") { fin >> m_smoothAlphaTemporal; }
        if (temp == "Magnitude") { fin >> m_spatialMagnitude; }
        if (temp == "Alpha") { fin >> m_smoothAlpha; }
        if (temp == "SHole") { fin >> m_spatialHoleFill; }
        if (temp == "MaxDistance") { fin >> m_maxDistance; }
        if (temp == "MinDistance") { fin >> m_minDistance; }
        if (temp == "HoleFill") { fin >> m_holeFill; }
        if (temp == "ViewCenter")
        {
            float x, y;
            fin >> x;
            fin >> y;
            sf::View view = m_game->window().getView();
            view.setCenter(x, y);
            fin >> temp;
            float sizeX, sizeY;
            fin >> sizeX;
            fin >> sizeY;
            view.setSize(sizeX, sizeY);
            m_game->window().setView(view);
        }
        if (temp == "m_drawContours") { fin >> m_drawContours; }
        if (temp == "m_numberOfContourLines") { fin >> m_numberOfContourLines; }
    }
    m_calibration.loadConfiguration();
}

sf::Color Scene_Sandbox::colorize(float height)
{
    int dNormal = height * 1529.f;
    int pR, pG, pB;
    int i = dNormal / 255;
    switch (i)
    {
    case 0: { pR = 255; pG = dNormal; pB = dNormal; } break;
    case 1: { pR = dNormal - 255; pG = 255; pB = dNormal;} break;
    case 2: { pR = 0; pG = 765 - dNormal; pB = dNormal; } break;
    case 3: { pR = 0; pG = 0; pB = dNormal - 765; } break;
    case 4: { pR = dNormal - 1020; pG = 0; pB = 255; } break;
    case 5: { pR = 255;pG = 0;pB = 1529 - dNormal; } break;
    }
    return sf::Color(pR, pG, pB);
}
