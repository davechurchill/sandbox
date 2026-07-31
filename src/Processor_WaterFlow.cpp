#include "Processor_WaterFlow.h"
#include "Profiler.hpp"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
    constexpr const char * WaterShaderPath = "shaders/shader_water_flow.frag";

    bool isTerrainCell(float height)
    {
        return std::isfinite(height) && height > 0.001f && height < 0.999f;
    }
}

void Processor_WaterFlow::init()
{
    reloadShader();
}

SandBoxProjector & Processor_WaterFlow::activeProjector()
{
    return m_overlayProcessor ? m_overlayProcessor->projector() : m_projector;
}

void Processor_WaterFlow::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(WaterShaderPath, sf::Shader::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
}

void Processor_WaterFlow::ensureSimulationSize(const cv::Size & size)
{
    if (m_water.size() == size)
    {
        return;
    }

    m_water = cv::Mat(size, CV_32F, 0.0f);
    m_nextWater = cv::Mat(size, CV_32F, 0.0f);
    m_wetness = cv::Mat(size, CV_32F, 0.0f);
}

void Processor_WaterFlow::resetWater()
{
    m_rainBrushActive = false;
    m_rainPulsePending = false;

    if (!m_water.empty())
    {
        m_water.setTo(0.0f);
        m_nextWater.setTo(0.0f);
        m_wetness.setTo(0.0f);
    }
}

void Processor_WaterFlow::addRain(const cv::Mat & terrain, float amount)
{
    if (m_rainMode == 0)
    {
        for (int y = 0; y < terrain.rows; y++)
        {
            const float * terrainRow = terrain.ptr<float>(y);
            float * waterRow = m_water.ptr<float>(y);
            for (int x = 0; x < terrain.cols; x++)
            {
                if (isTerrainCell(terrainRow[x]))
                {
                    waterRow[x] = std::min(waterRow[x] + amount, 1.0f);
                }
            }
        }
        return;
    }

    if (!m_rainBrushActive && !m_rainPulsePending)
    {
        return;
    }

    const float radius = std::max(m_rainRadius, 1.0f);
    const float radiusSquared = radius * radius;
    const int minX = std::max(0, (int)std::floor(m_rainBrushPosition.x - radius));
    const int maxX = std::min(terrain.cols - 1, (int)std::ceil(m_rainBrushPosition.x + radius));
    const int minY = std::max(0, (int)std::floor(m_rainBrushPosition.y - radius));
    const int maxY = std::min(terrain.rows - 1, (int)std::ceil(m_rainBrushPosition.y + radius));

    for (int y = minY; y <= maxY; y++)
    {
        const float * terrainRow = terrain.ptr<float>(y);
        float * waterRow = m_water.ptr<float>(y);
        for (int x = minX; x <= maxX; x++)
        {
            const float dx = x - m_rainBrushPosition.x;
            const float dy = y - m_rainBrushPosition.y;
            const float distanceSquared = dx * dx + dy * dy;
            if (distanceSquared <= radiusSquared && isTerrainCell(terrainRow[x]))
            {
                const float falloff = 1.0f - std::sqrt(distanceSquared) / radius;
                waterRow[x] = std::min(waterRow[x] + amount * falloff, 1.0f);
            }
        }
    }

    m_rainPulsePending = false;
}

void Processor_WaterFlow::simulate(const cv::Mat & terrain, float deltaTime)
{
    ensureSimulationSize(terrain.size());

    const float dt = std::clamp(deltaTime, 0.0f, 0.1f);
    for (int y = 0; y < terrain.rows; y++)
    {
        const float * terrainRow = terrain.ptr<float>(y);
        float * waterRow = m_water.ptr<float>(y);
        for (int x = 0; x < terrain.cols; x++)
        {
            if (!isTerrainCell(terrainRow[x]))
            {
                waterRow[x] = 0.0f;
            }
        }
    }

    addRain(terrain, m_rainfall * dt);

    const int steps = std::max(m_simulationSteps, 1);
    const float stepTime = dt / steps;
    const float evaporation = std::exp(-m_evaporation * stepTime);
    constexpr int offsets[4][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } };

    for (int step = 0; step < steps; step++)
    {
        m_water.copyTo(m_nextWater);

        for (int y = 0; y < terrain.rows; y++)
        {
            for (int x = 0; x < terrain.cols; x++)
            {
                const float terrainHeight = terrain.at<float>(y, x);
                const float waterAmount = m_water.at<float>(y, x);
                if (!isTerrainCell(terrainHeight) || waterAmount <= 0.0f)
                {
                    continue;
                }

                const float currentSurface = terrainHeight + waterAmount * m_waterDepthScale;
                float lowestSurface = currentSurface;
                int lowestX = x;
                int lowestY = y;

                for (const auto & offset : offsets)
                {
                    const int nx = x + offset[0];
                    const int ny = y + offset[1];
                    if (nx < 0 || nx >= terrain.cols || ny < 0 || ny >= terrain.rows)
                    {
                        continue;
                    }

                    const float neighborHeight = terrain.at<float>(ny, nx);
                    if (!isTerrainCell(neighborHeight))
                    {
                        continue;
                    }

                    const float neighborSurface = neighborHeight + m_water.at<float>(ny, nx) * m_waterDepthScale;
                    if (neighborSurface < lowestSurface)
                    {
                        lowestSurface = neighborSurface;
                        lowestX = nx;
                        lowestY = ny;
                    }
                }

                if (lowestX != x || lowestY != y)
                {
                    const float heightDifference = currentSurface - lowestSurface;
                    const float transfer = std::min(waterAmount, heightDifference * m_flowSpeed * stepTime);
                    m_nextWater.at<float>(y, x) -= transfer;
                    m_nextWater.at<float>(lowestY, lowestX) += transfer;
                }
            }
        }

        for (int y = 0; y < terrain.rows; y++)
        {
            const float * terrainRow = terrain.ptr<float>(y);
            float * nextRow = m_nextWater.ptr<float>(y);
            for (int x = 0; x < terrain.cols; x++)
            {
                nextRow[x] = isTerrainCell(terrainRow[x])
                    ? std::clamp(nextRow[x] * evaporation, 0.0f, 1.0f)
                    : 0.0f;
            }
        }

        std::swap(m_water, m_nextWater);
    }

    const float trailRetention = m_trailPersistence > 0.0f
        ? std::exp(-dt / m_trailPersistence)
        : 0.0f;

    for (int y = 0; y < terrain.rows; y++)
    {
        const float * waterRow = m_water.ptr<float>(y);
        float * wetnessRow = m_wetness.ptr<float>(y);
        for (int x = 0; x < terrain.cols; x++)
        {
            const float visibleWater = 1.0f - std::exp(-waterRow[x] * m_displayScale);
            wetnessRow[x] = std::clamp(std::max(visibleWater, wetnessRow[x] * trailRetention), 0.0f, 1.0f);
        }
    }
}

void Processor_WaterFlow::buildImage(const cv::Mat & terrain)
{
    cv::Mat terrain8u;
    terrain.convertTo(terrain8u, CV_8U, 255.0);

    cv::Mat currentWater(terrain.size(), CV_32F);
    for (int y = 0; y < terrain.rows; y++)
    {
        const float * waterRow = m_water.ptr<float>(y);
        float * visibleRow = currentWater.ptr<float>(y);
        for (int x = 0; x < terrain.cols; x++)
        {
            visibleRow[x] = std::clamp(1.0f - std::exp(-waterRow[x] * m_displayScale), 0.0f, 1.0f);
        }
    }

    cv::Mat wetness8u;
    cv::Mat currentWater8u;
    m_wetness.convertTo(wetness8u, CV_8U, 255.0);
    currentWater.convertTo(currentWater8u, CV_8U, 255.0);
    cv::Mat alpha(terrain.size(), CV_8U, cv::Scalar(255));

    std::vector<cv::Mat> channels = { terrain8u, wetness8u, currentWater8u, alpha };
    cv::merge(channels, m_encodedImage);
    activeProjector().project(m_encodedImage, m_projectedImage);

    if (m_projectedImage.empty())
    {
        m_hasFrame = false;
        return;
    }

    m_image.create(m_projectedImage.cols, m_projectedImage.rows, m_projectedImage.ptr());
    m_texture.loadFromImage(m_image);
    m_texture.setSmooth(true);
    m_sprite.setTexture(m_texture, true);
    m_hasFrame = true;
}

void Processor_WaterFlow::imgui()
{
    imguiControls(true);
}

void Processor_WaterFlow::imguiControls(bool showProjector)
{
    PROFILE_FUNCTION();

    const char * rainModes[] = { "Uniform Rain", "Rain Brush" };
    if (ImGui::Combo("Rain Mode", &m_rainMode, rainModes, IM_ARRAYSIZE(rainModes)))
    {
        m_rainBrushActive = false;
        m_rainPulsePending = false;
    }

    ImGui::SliderFloat("Rainfall Rate", &m_rainfall, 0.0f, 0.10f, "%.4f");
    if (m_rainMode == 1)
    {
        ImGui::SliderFloat("Rain Radius", &m_rainRadius, 1.0f, 128.0f);
        ImGui::TextUnformatted("Left mouse: release rain");
    }

    ImGui::SliderFloat("Flow Speed", &m_flowSpeed, 0.0f, 200.0f);
    ImGui::SliderFloat("Evaporation", &m_evaporation, 0.0f, 1.0f);
    ImGui::SliderFloat("Water Depth Scale", &m_waterDepthScale, 0.01f, 1.0f);
    ImGui::SliderInt("Simulation Steps", &m_simulationSteps, 1, 8);
    ImGui::SliderFloat("Trail Persistence", &m_trailPersistence, 0.0f, 10.0f);
    ImGui::SliderFloat("Water Visibility", &m_displayScale, 1.0f, 50.0f);
    ImGui::SliderFloat("Water Opacity", &m_waterOpacity, 0.0f, 1.0f);
    ImGui::ColorEdit3("Water Color", m_waterColor);

    if (ImGui::Button("Reset Water"))
    {
        resetWater();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }

    if (showProjector)
    {
        ImGui::Separator();
        m_projector.imgui();
    }
}

void Processor_WaterFlow::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();
    renderWater(window, false);
}

void Processor_WaterFlow::renderWater(sf::RenderWindow & window, bool overlayOnly)
{
    if (m_hasFrame)
    {
        SandBoxProjector & projector = activeProjector();
        m_sprite.setPosition(projector.getTransformedPosition());
        const float scale = projector.getTransformedScale();
        m_sprite.setScale(scale, scale);

        if (m_shaderLoaded)
        {
            m_shader.setUniform("waterColor", sf::Glsl::Vec3(m_waterColor[0], m_waterColor[1], m_waterColor[2]));
            m_shader.setUniform("waterOpacity", m_waterOpacity);
            m_shader.setUniform("overlayOnly", overlayOnly);
            window.draw(m_sprite, &m_shader);
        }
        else if (!overlayOnly)
        {
            window.draw(m_sprite);
        }
    }
}

void Processor_WaterFlow::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    const bool draggingProjection = activeProjector().processEvent(event, mouse);

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
    {
        m_rainBrushActive = false;
        return;
    }

    if (m_rainMode != 1 || draggingProjection || ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
    {
        m_rainBrushActive = mapMouseToTerrain(mouse, m_rainBrushPosition);
        m_rainPulsePending = m_rainBrushActive;
    }
    else if (event.type == sf::Event::MouseMoved && m_rainBrushActive)
    {
        if (!mapMouseToTerrain(mouse, m_rainBrushPosition))
        {
            m_rainBrushActive = false;
        }
    }
}

bool Processor_WaterFlow::mapMouseToTerrain(const sf::Vector2f & mouse, cv::Point2f & terrainPosition)
{
    if (m_water.empty())
    {
        return false;
    }

    SandBoxProjector & projector = activeProjector();
    const float scale = projector.getTransformedScale();
    if (!std::isfinite(scale) || scale <= 0.0f)
    {
        return false;
    }

    const sf::Vector2f offset = mouse - projector.getTransformedPosition();
    std::vector<cv::Point2f> point = { { offset.x / scale, offset.y / scale } };
    const cv::Mat projection = projector.getProjectionMatrix();
    if (projection.empty())
    {
        return false;
    }

    cv::Mat inverseProjection;
    if (cv::invert(projection, inverseProjection) == 0.0)
    {
        return false;
    }

    cv::perspectiveTransform(point, point, inverseProjection);
    terrainPosition = point.front();
    return std::isfinite(terrainPosition.x) && std::isfinite(terrainPosition.y)
        && terrainPosition.x >= 0.0f && terrainPosition.x < m_water.cols
        && terrainPosition.y >= 0.0f && terrainPosition.y < m_water.rows;
}

void Processor_WaterFlow::save(Save & save) const
{
    m_projector.save(save);
}

void Processor_WaterFlow::load(const Save & save)
{
    m_projector.load(save);
}

void Processor_WaterFlow::processTopography(const IntermediateData & data)
{
    PROFILE_FUNCTION();

    if (data.topography.empty() || data.topography.type() != CV_32F)
    {
        m_hasFrame = false;
        return;
    }

    simulate(data.topography, data.deltaTime);
    buildImage(data.topography);
}

void Processor_WaterFlow::initOverlay()
{
    resetWater();
    reloadShader();
}

void Processor_WaterFlow::imguiOverlay()
{
    imguiControls(false);
}

void Processor_WaterFlow::processTopographyOverlay(
    const IntermediateData & data,
    TopographyProcessor & processor)
{
    if (data.topography.empty() || data.topography.type() != CV_32F)
    {
        m_hasFrame = false;
        return;
    }

    m_overlayProcessor = &processor;
    simulate(data.topography, data.deltaTime);
    buildImage(data.topography);
}

void Processor_WaterFlow::renderOverlay(
    sf::RenderWindow & window,
    TopographyProcessor & processor)
{
    m_overlayProcessor = &processor;
    renderWater(window, true);
}

void Processor_WaterFlow::processOverlayEvent(
    const sf::Event & event,
    const sf::Vector2f & mouse,
    TopographyProcessor & processor)
{
    m_overlayProcessor = &processor;
    processEvent(event, mouse);
}

void Processor_WaterFlow::saveOverlay(Save &) const
{
}

void Processor_WaterFlow::loadOverlay(const Save &)
{
}
