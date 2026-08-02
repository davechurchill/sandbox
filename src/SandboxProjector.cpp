#include "SandboxProjector.h"
#include "Profiler.hpp"
#include "imgui-SFML.h"
#include "imgui.h"
#include "Tools.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <cmath>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp> 

SandboxProjector::SandboxProjector()
{
    m_projectionCircles = std::vector<sf::CircleShape>(4, sf::CircleShape(m_handleSize, 64));
    updateProjectionHandles();
}

sf::FloatRect SandboxProjector::projectionBounds() const
{
    sf::Vector2f minimum{ m_projectionPoints[0].x, m_projectionPoints[0].y };
    sf::Vector2f maximum = minimum;
    for (const cv::Point2f & point : m_projectionPoints)
    {
        minimum.x = std::min(minimum.x, point.x);
        minimum.y = std::min(minimum.y, point.y);
        maximum.x = std::max(maximum.x, point.x);
        maximum.y = std::max(maximum.y, point.y);
    }
    return { minimum, maximum - minimum };
}

void SandboxProjector::project(const cv::Mat & input, cv::Mat & output)
{
    if (input.empty())
    {
        output.release();
        return;
    }

    // Check to see if data matrix has changed in size and generate the projection matrix again if so
    int width = input.cols;
    int height = input.rows;
    if (width != m_dataWidth || height != m_dataHeight)
    {
        m_dataWidth = width;
        m_dataHeight = height;
        regenerateProjection();
    }

    if (!m_projectionValid || m_projectionMatrix.empty())
    {
        output.release();
        return;
    }

    // Apply projection
    cv::warpPerspective(input, output, m_projectionMatrix, cv::Size(m_finalWidth, m_finalHeight));
}

void SandboxProjector::setTerrain(const cv::Mat & source, std::uint64_t terrainRevision)
{
    PROFILE_FUNCTION();

    const bool terrainChanged = terrainRevision != m_terrainRevision
        || source.data != m_terrainSource.data
        || source.size() != m_terrainSource.size()
        || source.type() != m_terrainSource.type();
    m_terrainRevision = terrainRevision;

    if (source.empty())
    {
        m_terrainSource.release();
        m_hasTerrainTexture = false;
        m_terrainTextureNeedsUpdate = false;
        if (m_dataWidth != 0 || m_dataHeight != 0)
        {
            m_dataWidth = 0;
            m_dataHeight = 0;
            regenerateProjection();
            m_terrainTextureNeedsUpdate = false;
        }
        return;
    }

    m_terrainSource = source;
    if (source.cols != m_dataWidth || source.rows != m_dataHeight)
    {
        m_dataWidth = source.cols;
        m_dataHeight = source.rows;
        regenerateProjection();
    }
    else if (terrainChanged)
    {
        m_terrainTextureNeedsUpdate = true;
    }
}

bool SandboxProjector::ensureTerrainTexture()
{
    if (m_terrainSource.empty())
    {
        m_hasTerrainTexture = false;
        return false;
    }

    if (!m_terrainTextureNeedsUpdate)
    {
        return m_hasTerrainTexture;
    }

    if (m_terrainSource.cols != m_dataWidth || m_terrainSource.rows != m_dataHeight)
    {
        m_dataWidth = m_terrainSource.cols;
        m_dataHeight = m_terrainSource.rows;
        generateProjection();
    }

    m_terrainTextureNeedsUpdate = false;
    m_hasTerrainTexture = false;

    if (m_terrainSource.type() != CV_32F
        || !m_projectionValid || m_projectionMatrix.empty())
    {
        return false;
    }

    try
    {
        {
            PROFILE_SCOPE("Calibration TransformProjection");
            cv::warpPerspective(
                m_terrainSource,
                m_projectedTerrain,
                m_projectionMatrix,
                cv::Size(m_finalWidth, m_finalHeight));
        }
        if (m_projectedTerrain.empty())
        {
            return false;
        }

        {
            PROFILE_SCOPE("Convert Projected Terrain");
            m_projectedTerrain.convertTo(m_terrainBytes, CV_8U, 255.0);
            cv::cvtColor(m_terrainBytes, m_terrainRgba, cv::COLOR_GRAY2RGBA);
            if (!m_terrainRgba.isContinuous())
            {
                m_terrainRgba = m_terrainRgba.clone();
            }
        }
    }
    catch (const cv::Exception & error)
    {
        std::cerr << "Failed to prepare the shared projected terrain: " << error.what() << '\n';
        return false;
    }

    const sf::Vector2u textureSize(
        (unsigned int)m_terrainRgba.cols,
        (unsigned int)m_terrainRgba.rows);
    if (m_terrainTexture.getSize() != textureSize)
    {
        if (!m_terrainTexture.resize(textureSize))
        {
            std::cerr << "Failed to resize the shared projected-terrain texture.\n";
            m_hasTerrainTexture = false;
            return false;
        }
        m_terrainSprite.setTexture(m_terrainTexture, true);
    }

    {
        PROFILE_SCOPE("Upload Projected Terrain");
        m_terrainTexture.update(m_terrainRgba.ptr());
    }

    m_hasTerrainTexture = true;
    return true;
}

bool SandboxProjector::drawTerrain(sf::RenderWindow & window, sf::Shader * shader, bool smooth)
{
    if (!ensureTerrainTexture())
    {
        return false;
    }

    if (m_terrainTexture.isSmooth() != smooth)
    {
        m_terrainTexture.setSmooth(smooth);
    }
    m_terrainSprite.setPosition(getTransformedPosition());
    const float scale = getTransformedScale();
    m_terrainSprite.setScale({ scale, scale });
    if (shader)
    {
        window.draw(m_terrainSprite, shader);
    }
    else
    {
        window.draw(m_terrainSprite);
    }
    return true;
}

const sf::Texture * SandboxProjector::terrainTexture(bool smooth)
{
    if (!ensureTerrainTexture())
    {
        return nullptr;
    }
    if (m_terrainTexture.isSmooth() != smooth)
    {
        m_terrainTexture.setSmooth(smooth);
    }
    return &m_terrainTexture;
}

void SandboxProjector::imgui()
{
    PROFILE_FUNCTION();

    ImGui::Checkbox("Show Projection", &m_drawProjection);
    ImGui::Checkbox("Align Depth Map with Projection", &m_drawProjectedDepthMap);
    ImGui::Checkbox("Show Projection Lines", &m_drawLines);
    ImGui::Checkbox("Show Calibration Grid", &m_drawGrid);
    if (m_drawGrid)
    {
        ImGui::SliderInt("Grid Divisions", &m_gridDivisions, 2, 32);
    }

    if (ImGui::Button("Reset Corners"))
    {
        resetProjectionPoints();
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Corner Coordinates");
    const char * cornerLabels[] = {
        "Top Left",
        "Top Right",
        "Bottom Left",
        "Bottom Right"
    };
    bool pointsChanged = false;
    for (size_t index = 0; index < std::size(m_projectionPoints); index++)
    {
        pointsChanged |= ImGui::InputFloat2(
            cornerLabels[index],
            &m_projectionPoints[index].x,
            "%.1f");
    }
    if (pointsChanged)
    {
        updateProjectionHandles();
        regenerateProjection();
    }

    ImGui::Separator();
    const char * rotations[] = {
        "0 degrees",
        "90 degrees clockwise",
        "180 degrees",
        "270 degrees clockwise"
    };
    bool orientationChanged = ImGui::Combo(
        "Rotation",
        &m_rotationQuarterTurns,
        rotations,
        IM_ARRAYSIZE(rotations));
    orientationChanged |= ImGui::Checkbox("Mirror Horizontally", &m_mirrorHorizontal);
    orientationChanged |= ImGui::Checkbox("Mirror Vertically", &m_mirrorVertical);
    if (orientationChanged)
    {
        regenerateProjection();
    }

    ImGui::Separator();
    if (ImGui::SliderFloat("Handle Size", &m_handleSize, 3.0f, 30.0f, "%.0f px"))
    {
        updateProjectionHandles();
    }
    ImGui::ColorEdit3("Line Color", m_lineColor);
    ImGui::SliderFloat("Line Opacity", &m_lineOpacity, 0.0f, 1.0f);
}

void SandboxProjector::resetProjectionPoints()
{
    m_dragPoint = -1;
    m_projectionPoints[0] = { 400.0f, 400.0f };
    m_projectionPoints[1] = { 500.0f, 400.0f };
    m_projectionPoints[2] = { 400.0f, 500.0f };
    m_projectionPoints[3] = { 500.0f, 500.0f };
    updateProjectionHandles();
    regenerateProjection();
}

void SandboxProjector::updateProjectionHandles()
{
    for (size_t index = 0; index < m_projectionCircles.size(); index++)
    {
        sf::CircleShape & circle = m_projectionCircles[index];
        circle.setRadius(m_handleSize);
        circle.setOrigin({ m_handleSize, m_handleSize });
        circle.setFillColor(sf::Color::Magenta);
        circle.setPosition({ m_projectionPoints[index].x, m_projectionPoints[index].y });
    }
}

bool SandboxProjector::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    PROFILE_FUNCTION();

    if (!m_drawLines)
    {
        m_dragPoint = -1;
        return false;
    }

    // detect if we have clicked a circle
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>();
        mousePressed && mousePressed->button == sf::Mouse::Button::Left)
    {
        m_dragPoint = Tools::GetClickedCircleIndex(mouse.x, mouse.y, m_projectionCircles);
    }

    // if we have released the mouse button
    if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>();
        mouseReleased && mouseReleased->button == sf::Mouse::Button::Left)
    {
        m_dragPoint = -1;
    }

    // if the mouse moved and we are dragging something, update its position and regenerate the matrix
    if (event.is<sf::Event::MouseMoved>())
    {
        if (m_dragPoint != -1)
        {
            m_projectionPoints[m_dragPoint] = cv::Point((int)mouse.x, (int)mouse.y);
            m_projectionCircles[m_dragPoint].setPosition(mouse);
            regenerateProjection();
        }
    }

    return m_dragPoint != -1;
}

bool SandboxProjector::unprojectPoint(
    const sf::Vector2f & point,
    sf::Vector2f & dataPoint)
{
    const float scale = getTransformedScale();
    if (!m_projectionValid || m_projectionMatrix.empty()
        || !std::isfinite(scale) || scale <= 0.0f)
    {
        return false;
    }

    cv::Mat inverseProjection;
    if (cv::invert(m_projectionMatrix, inverseProjection) == 0.0)
    {
        return false;
    }

    const sf::Vector2f local = (point - m_minXY) / scale;
    std::vector<cv::Point2f> projectedPoint = { { local.x, local.y } };
    cv::perspectiveTransform(projectedPoint, projectedPoint, inverseProjection);
    if (projectedPoint.empty()
        || !std::isfinite(projectedPoint[0].x)
        || !std::isfinite(projectedPoint[0].y))
    {
        return false;
    }

    dataPoint = { projectedPoint[0].x, projectedPoint[0].y };
    return true;
}

void SandboxProjector::render(sf::RenderWindow & window)
{
    if (!m_drawLines) { return; }
    PROFILE_FUNCTION();

    const auto colorChannel = [](float value)
    {
        return (std::uint8_t)std::round(std::clamp(value, 0.0f, 1.0f) * 255.0f);
    };
    const sf::Color lineColor(
        colorChannel(m_lineColor[0]),
        colorChannel(m_lineColor[1]),
        colorChannel(m_lineColor[2]),
        colorChannel(m_lineOpacity));

    if (m_drawGrid)
    {
        const auto interpolate = [](const cv::Point2f & start, const cv::Point2f & end, float amount)
        {
            return sf::Vector2f(
                start.x + (end.x - start.x) * amount,
                start.y + (end.y - start.y) * amount);
        };

        sf::VertexArray gridVertices(sf::PrimitiveType::Lines);
        for (int division = 1; division < m_gridDivisions; division++)
        {
            const float amount = (float)division / m_gridDivisions;
            gridVertices.append(sf::Vertex(interpolate(m_projectionPoints[0], m_projectionPoints[1], amount), lineColor));
            gridVertices.append(sf::Vertex(interpolate(m_projectionPoints[2], m_projectionPoints[3], amount), lineColor));
            gridVertices.append(sf::Vertex(interpolate(m_projectionPoints[0], m_projectionPoints[2], amount), lineColor));
            gridVertices.append(sf::Vertex(interpolate(m_projectionPoints[1], m_projectionPoints[3], amount), lineColor));
        }
        window.draw(gridVertices);
    }

    for (size_t i = 0; i < m_projectionCircles.size(); ++i)
    {
        m_projectionCircles[i].setPosition({ m_projectionPoints[i].x, m_projectionPoints[i].y });
        window.draw(m_projectionCircles[i]);
    }

    sf::VertexArray projectionVertices(sf::PrimitiveType::LineStrip);
    projectionVertices.append(sf::Vertex(m_projectionCircles[0].getPosition(), lineColor));
    projectionVertices.append(sf::Vertex(m_projectionCircles[1].getPosition(), lineColor));
    projectionVertices.append(sf::Vertex(m_projectionCircles[3].getPosition(), lineColor));
    projectionVertices.append(sf::Vertex(m_projectionCircles[2].getPosition(), lineColor));
    projectionVertices.append(sf::Vertex(m_projectionCircles[0].getPosition(), lineColor));
    window.draw(projectionVertices);
}

void SandboxProjector::generateProjection()
{
    PROFILE_FUNCTION();

    m_projectionValid = false;
    m_projectionMatrix.release();
    m_finalWidth = 0;
    m_finalHeight = 0;
    if (m_dataWidth <= 0 || m_dataHeight <= 0) { return; }

    std::array<cv::Point2f, 4> dataCorners = {
        cv::Point2f(0, 0),
        cv::Point2f((float)m_dataWidth, 0),
        cv::Point2f(0, (float)m_dataHeight),
        cv::Point2f((float)m_dataWidth, (float)m_dataHeight),
    };

    if (m_mirrorHorizontal)
    {
        std::swap(dataCorners[0], dataCorners[1]);
        std::swap(dataCorners[2], dataCorners[3]);
    }
    if (m_mirrorVertical)
    {
        std::swap(dataCorners[0], dataCorners[2]);
        std::swap(dataCorners[1], dataCorners[3]);
    }
    for (int turn = 0; turn < m_rotationQuarterTurns; turn++)
    {
        const std::array<cv::Point2f, 4> previousCorners = dataCorners;
        dataCorners = {
            previousCorners[2],
            previousCorners[0],
            previousCorners[3],
            previousCorners[1]
        };
    }

    cv::Point2f boxPoints[] = { m_projectionPoints[0], m_projectionPoints[1], m_projectionPoints[2], m_projectionPoints[3] };

    sf::Vector2f minXY = { boxPoints[0].x, boxPoints[0].y };
    float maxX = boxPoints[0].x;
    float maxY = boxPoints[0].y;
    for (int i = 0; i < 4; i++)
    {
        if (boxPoints[i].x < minXY.x)   { minXY.x = boxPoints[i].x; }
        if (boxPoints[i].x > maxX)      { maxX = boxPoints[i].x; }
        if (boxPoints[i].y < minXY.y)   { minXY.y = boxPoints[i].y; }
        if (boxPoints[i].y > maxY)      { maxY = boxPoints[i].y; }
    }

    for (int i = 0; i < 4; i++)
    {
        boxPoints[i].x -= minXY.x;
        boxPoints[i].y -= minXY.y;
    }
    int boxWidth = (int)(maxX - minXY.x);
    int boxHeight = (int)(maxY - minXY.y);
    std::vector<cv::Point2f> polygon = { boxPoints[0], boxPoints[1], boxPoints[3], boxPoints[2] };
    if (boxWidth < 1 || boxHeight < 1 || std::abs(cv::contourArea(polygon)) < 1.0)
    {
        return;
    }

    m_minXY = minXY;
    float ratio = (float)boxHeight / boxWidth;
    m_finalWidth = (int)(m_dataWidth * 1.5f);
    m_finalHeight = (int)(m_finalWidth * ratio);
    m_boxScale = sf::Vector2f((float)m_finalWidth / boxWidth, (float)m_finalHeight / boxHeight);

    for (int i = 0; i < 4; i++)
    {
        boxPoints[i].x *= m_boxScale.x;
        boxPoints[i].y *= m_boxScale.y;
    }

    try
    {
        m_projectionMatrix = cv::getPerspectiveTransform(dataCorners.data(), boxPoints);
        m_projectionValid = !m_projectionMatrix.empty();
    }
    catch (const cv::Exception & error)
    {
        std::cerr << "Failed to generate the terrain projection: " << error.what() << '\n';
    }
}

void SandboxProjector::regenerateProjection()
{
    generateProjection();
    m_terrainTextureNeedsUpdate = true;
}

void SandboxProjector::save(Settings& save) const
{
    Settings::json & settings = save.section("Projection");
    settings["m_projectionPoints"] = Settings::json::array();
    for (const cv::Point2f & point : m_projectionPoints)
    {
        settings["m_projectionPoints"].push_back({ point.x, point.y });
    }
    settings["m_drawLines"] = m_drawLines;
    settings["m_drawProjection"] = m_drawProjection;
    settings["m_drawProjectedDepthMap"] = m_drawProjectedDepthMap;
    settings["m_drawGrid"] = m_drawGrid;
    settings["m_gridDivisions"] = m_gridDivisions;
    settings["m_rotationQuarterTurns"] = m_rotationQuarterTurns;
    settings["m_mirrorHorizontal"] = m_mirrorHorizontal;
    settings["m_mirrorVertical"] = m_mirrorVertical;
    settings["m_handleSize"] = m_handleSize;
    settings["m_lineColor"] = { m_lineColor[0], m_lineColor[1], m_lineColor[2] };
    settings["m_lineOpacity"] = m_lineOpacity;
}

void SandboxProjector::load(const Settings& save)
{
    const Settings::json & settings = save.section("Projection");
    const auto points = settings.find("m_projectionPoints");
    if (points != settings.end() && points->is_array() && points->size() == 4)
    {
        for (size_t index = 0; index < 4; index++)
        {
            const Settings::json & point = points->at(index);
            if (point.is_array() && point.size() == 2)
            {
                m_projectionPoints[index] = { point[0].get<float>(), point[1].get<float>() };
            }
        }
    }
    Settings::read(settings, "m_drawLines", m_drawLines);
    Settings::read(settings, "m_drawProjection", m_drawProjection);
    Settings::read(settings, "m_drawProjectedDepthMap", m_drawProjectedDepthMap);
    Settings::read(settings, "m_drawGrid", m_drawGrid);
    Settings::read(settings, "m_gridDivisions", m_gridDivisions);
    Settings::read(settings, "m_rotationQuarterTurns", m_rotationQuarterTurns);
    Settings::read(settings, "m_mirrorHorizontal", m_mirrorHorizontal);
    Settings::read(settings, "m_mirrorVertical", m_mirrorVertical);
    Settings::read(settings, "m_handleSize", m_handleSize);
    const auto color = settings.find("m_lineColor");
    if (color != settings.end() && color->is_array() && color->size() == 3)
    {
        for (size_t index = 0; index < 3; index++)
        {
            m_lineColor[index] = color->at(index).get<float>();
        }
    }
    Settings::read(settings, "m_lineOpacity", m_lineOpacity);

    m_gridDivisions = std::clamp(m_gridDivisions, 2, 32);
    m_rotationQuarterTurns = std::clamp(m_rotationQuarterTurns, 0, 3);
    m_handleSize = std::clamp(m_handleSize, 3.0f, 30.0f);
    for (float & channel : m_lineColor)
    {
        channel = std::clamp(channel, 0.0f, 1.0f);
    }
    m_lineOpacity = std::clamp(m_lineOpacity, 0.0f, 1.0f);
    updateProjectionHandles();
    regenerateProjection();
}
