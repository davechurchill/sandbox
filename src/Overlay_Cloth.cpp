#include "Overlay_Cloth.h"

#include "Profiler.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    bool isTerrainHeight(float height)
    {
        return std::isfinite(height) && height > 0.001f && height < 0.999f;
    }

}

float Overlay_Cloth::vectorLength(const Vector3 & vector)
{
    return std::sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

int Overlay_Cloth::nodeIndex(int column, int row) const
{
    return row * (m_cellsX + 1) + column;
}

float Overlay_Cloth::sampleHeight(const cv::Point2f & position) const
{
    if (m_topography.empty() || m_topography.type() != CV_32F
        || position.x < 0.0f || position.y < 0.0f
        || position.x >= m_topography.cols || position.y >= m_topography.rows)
    {
        return std::numeric_limits<float>::quiet_NaN();
    }

    const int x = std::clamp((int)std::round(position.x), 0, m_topography.cols - 1);
    const int y = std::clamp((int)std::round(position.y), 0, m_topography.rows - 1);
    const float height = m_topography.at<float>(y, x);
    return isTerrainHeight(height)
        ? height
        : std::numeric_limits<float>::quiet_NaN();
}

float Overlay_Cloth::heightScale() const
{
    return std::max(1.0f, std::min(m_topography.cols, m_topography.rows) * 0.35f);
}

void Overlay_Cloth::addSpring(int first, int second, float stiffnessScale)
{
    const Vector3 delta{
        m_nodes[second].position.x - m_nodes[first].position.x,
        m_nodes[second].position.y - m_nodes[first].position.y,
        m_nodes[second].position.z - m_nodes[first].position.z };
    m_springs.push_back({ first, second, vectorLength(delta), stiffnessScale });
}

void Overlay_Cloth::createCloth()
{
    if (m_topography.empty())
    {
        return;
    }

    if (!m_centerSet)
    {
        m_center = { m_topography.cols * 0.5f, m_topography.rows * 0.5f };
        m_centerSet = true;
    }

    m_nodes.clear();
    m_springs.clear();
    const int columns = m_cellsX + 1;
    const int rows = m_cellsY + 1;
    m_nodes.resize(columns * rows);

    const float width = std::min(m_clothSize, std::min(m_topography.cols, m_topography.rows) * 0.96f);
    const float clothHeight = width * 0.68f;
    const float halfWidth = width * 0.5f;
    const float halfHeight = clothHeight * 0.5f;
    const float maximumCenterX = m_topography.cols - 1.0f - halfWidth;
    const float maximumCenterY = m_topography.rows - 1.0f - halfHeight;
    m_center.x = maximumCenterX >= halfWidth
        ? std::clamp(m_center.x, halfWidth, maximumCenterX)
        : (m_topography.cols - 1.0f) * 0.5f;
    m_center.y = maximumCenterY >= halfHeight
        ? std::clamp(m_center.y, halfHeight, maximumCenterY)
        : (m_topography.rows - 1.0f) * 0.5f;
    const float initialHeight = heightScale() * 1.12f;
    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            const float xAmount = (float)column / m_cellsX - 0.5f;
            const float yAmount = (float)row / m_cellsY - 0.5f;
            Node & node = m_nodes[nodeIndex(column, row)];
            node.position = {
                std::clamp(m_center.x + xAmount * width, 0.0f, m_topography.cols - 1.0f),
                std::clamp(m_center.y + yAmount * clothHeight, 0.0f, m_topography.rows - 1.0f),
                initialHeight };
        }
    }

    for (int row = 0; row < rows; row++)
    {
        for (int column = 0; column < columns; column++)
        {
            const int current = nodeIndex(column, row);
            if (column + 1 < columns)
            {
                addSpring(current, nodeIndex(column + 1, row), 1.0f);
            }
            if (row + 1 < rows)
            {
                addSpring(current, nodeIndex(column, row + 1), 1.0f);
            }
            if (column + 1 < columns && row + 1 < rows)
            {
                addSpring(current, nodeIndex(column + 1, row + 1), 0.72f);
                addSpring(nodeIndex(column + 1, row), nodeIndex(column, row + 1), 0.72f);
            }
            if (column + 2 < columns)
            {
                addSpring(current, nodeIndex(column + 2, row), 0.28f);
            }
            if (row + 2 < rows)
            {
                addSpring(current, nodeIndex(column, row + 2), 0.28f);
            }
        }
    }

    m_resetPending = false;
}

void Overlay_Cloth::updateCloth(float deltaTime)
{
    if (m_nodes.empty())
    {
        return;
    }

    constexpr int Substeps = 4;
    const float dt = std::clamp(deltaTime, 0.0f, 0.05f) / Substeps;
    const float verticalScale = heightScale();
    const float globalDamping = std::exp(-m_damping * dt);
    std::vector<Vector3> acceleration(m_nodes.size());

    for (int step = 0; step < Substeps; step++)
    {
        for (Vector3 & value : acceleration)
        {
            value = {
                m_windX * verticalScale * 0.18f,
                m_windY * verticalScale * 0.18f,
                -m_gravity * verticalScale * 0.72f };
        }

        for (const Spring & spring : m_springs)
        {
            Node & first = m_nodes[spring.first];
            Node & second = m_nodes[spring.second];
            const Vector3 delta{
                second.position.x - first.position.x,
                second.position.y - first.position.y,
                second.position.z - first.position.z };
            const float length = vectorLength(delta);
            if (length <= 0.0001f)
            {
                continue;
            }

            const Vector3 direction{ delta.x / length, delta.y / length, delta.z / length };
            const Vector3 relativeVelocity{
                second.velocity.x - first.velocity.x,
                second.velocity.y - first.velocity.y,
                second.velocity.z - first.velocity.z };
            const float alongSpring = relativeVelocity.x * direction.x
                + relativeVelocity.y * direction.y
                + relativeVelocity.z * direction.z;
            const float springForce = std::clamp(
                (length - spring.restLength) * m_stiffness * spring.stiffnessScale
                    + alongSpring * m_damping * 0.32f,
                -verticalScale * 16.0f,
                verticalScale * 16.0f);

            acceleration[spring.first].x += direction.x * springForce;
            acceleration[spring.first].y += direction.y * springForce;
            acceleration[spring.first].z += direction.z * springForce;
            acceleration[spring.second].x -= direction.x * springForce;
            acceleration[spring.second].y -= direction.y * springForce;
            acceleration[spring.second].z -= direction.z * springForce;
        }

        for (size_t i = 0; i < m_nodes.size(); i++)
        {
            Node & node = m_nodes[i];
            node.velocity.x = (node.velocity.x + acceleration[i].x * dt) * globalDamping;
            node.velocity.y = (node.velocity.y + acceleration[i].y * dt) * globalDamping;
            node.velocity.z = (node.velocity.z + acceleration[i].z * dt) * globalDamping;
            node.position.x += node.velocity.x * dt;
            node.position.y += node.velocity.y * dt;
            node.position.z += node.velocity.z * dt;

            node.position.x = std::clamp(node.position.x, 0.0f, m_topography.cols - 1.0f);
            node.position.y = std::clamp(node.position.y, 0.0f, m_topography.rows - 1.0f);
            const float terrainHeight = sampleHeight({ node.position.x, node.position.y });
            const float terrainZ = (std::isfinite(terrainHeight) ? terrainHeight : 0.0f) * verticalScale;
            if (node.position.z < terrainZ + 1.0f)
            {
                node.position.z = terrainZ + 1.0f;
                if (node.velocity.z < 0.0f)
                {
                    node.velocity.z *= -0.08f;
                }
                node.velocity.x *= 0.72f;
                node.velocity.y *= 0.72f;
            }
        }
    }
}

bool Overlay_Cloth::mapMouseToTerrain(
    const sf::Vector2f & mouse,
    TopographyProcessor & processor,
    cv::Point2f & terrainPosition) const
{
    if (m_topography.empty())
    {
        return false;
    }

    SandBoxProjector & projector = processor.projector();
    const float scale = projector.getTransformedScale();
    const cv::Mat projection = projector.getProjectionMatrix();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return false;
    }

    cv::Mat inverseProjection;
    if (cv::invert(projection, inverseProjection) == 0.0)
    {
        return false;
    }

    const sf::Vector2f local = (mouse - projector.getTransformedPosition()) / scale;
    std::vector<cv::Point2f> point = { { local.x, local.y } };
    cv::perspectiveTransform(point, point, inverseProjection);
    terrainPosition = point.front();
    return terrainPosition.x >= 0.0f && terrainPosition.y >= 0.0f
        && terrainPosition.x < m_topography.cols && terrainPosition.y < m_topography.rows;
}

void Overlay_Cloth::renderCloth(
    sf::RenderWindow & window,
    TopographyProcessor & processor) const
{
    if (m_nodes.empty())
    {
        return;
    }

    SandBoxProjector & projector = processor.projector();
    const cv::Mat projection = projector.getProjectionMatrix();
    const float scale = projector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    std::vector<cv::Point2f> projectedPoints;
    projectedPoints.reserve(m_nodes.size());
    for (const Node & node : m_nodes)
    {
        projectedPoints.push_back({ node.position.x, node.position.y });
    }
    cv::perspectiveTransform(projectedPoints, projectedPoints, projection);

    const sf::Vector2f origin = projector.getTransformedPosition();
    const float verticalScale = heightScale();
    std::vector<sf::Vector2f> screenPoints(m_nodes.size());
    for (size_t i = 0; i < m_nodes.size(); i++)
    {
        const float terrainHeight = sampleHeight({ m_nodes[i].position.x, m_nodes[i].position.y });
        const float terrainZ = (std::isfinite(terrainHeight) ? terrainHeight : 0.0f) * verticalScale;
        const float clearance = std::max(0.0f, m_nodes[i].position.z - terrainZ);
        screenPoints[i] = {
            origin.x + projectedPoints[i].x * scale,
            origin.y + projectedPoints[i].y * scale - clearance * scale * 0.42f };
    }

    sf::VertexArray fabric(sf::PrimitiveType::Triangles);
    const std::uint8_t fabricAlpha = (std::uint8_t)std::clamp(
        (int)std::round((1.0f - m_sheetTransparency) * 255.0f),
        0,
        255);
    for (int row = 0; row < m_cellsY; row++)
    {
        for (int column = 0; column < m_cellsX; column++)
        {
            const int topLeft = nodeIndex(column, row);
            const int topRight = nodeIndex(column + 1, row);
            const int bottomLeft = nodeIndex(column, row + 1);
            const int bottomRight = nodeIndex(column + 1, row + 1);
            const sf::Color color = ((column + row) & 1)
                ? sf::Color(48, 126, 185, fabricAlpha)
                : sf::Color(68, 164, 214, fabricAlpha);
            fabric.append(sf::Vertex(screenPoints[topLeft], color));
            fabric.append(sf::Vertex(screenPoints[topRight], color));
            fabric.append(sf::Vertex(screenPoints[bottomLeft], color));
            fabric.append(sf::Vertex(screenPoints[topRight], color));
            fabric.append(sf::Vertex(screenPoints[bottomRight], color));
            fabric.append(sf::Vertex(screenPoints[bottomLeft], color));
        }
    }
    window.draw(fabric, sf::BlendAlpha);

    sf::VertexArray grid(sf::PrimitiveType::Lines);
    std::vector<sf::Color> gridColors(m_nodes.size());
    for (size_t i = 0; i < m_nodes.size(); i++)
    {
        const float normalizedHeight = std::clamp(m_nodes[i].position.z / verticalScale, 0.0f, 1.0f);
        const float brightness = 0.78f + normalizedHeight * 0.22f;
        gridColors[i] = sf::Color(
            (std::uint8_t)(205.0f * brightness),
            (std::uint8_t)(235.0f * brightness),
            (std::uint8_t)(247.0f * brightness),
            255);
    }
    for (int row = 0; row <= m_cellsY; row++)
    {
        for (int column = 0; column <= m_cellsX; column++)
        {
            const int current = nodeIndex(column, row);
            if (column < m_cellsX)
            {
                const int right = nodeIndex(column + 1, row);
                grid.append(sf::Vertex(screenPoints[current], gridColors[current]));
                grid.append(sf::Vertex(screenPoints[right], gridColors[right]));
            }
            if (row < m_cellsY)
            {
                const int below = nodeIndex(column, row + 1);
                grid.append(sf::Vertex(screenPoints[current], gridColors[current]));
                grid.append(sf::Vertex(screenPoints[below], gridColors[below]));
            }
        }
    }
    window.draw(grid, sf::BlendAlpha);
}

void Overlay_Cloth::initOverlay()
{
    m_nodes.clear();
    m_springs.clear();
    m_centerSet = false;
    m_resetPending = true;
}

void Overlay_Cloth::imguiOverlay()
{
    PROFILE_FUNCTION();

    bool cellsChanged = ImGui::SliderInt("Cells X", &m_cellsX, 2, 64);
    cellsChanged |= ImGui::SliderInt("Cells Y", &m_cellsY, 2, 64);
    if (cellsChanged)
    {
        m_resetPending = true;
    }
    if (ImGui::SliderFloat("Cloth Size", &m_clothSize, 60.0f, 500.0f, "%.0f px"))
    {
        m_resetPending = true;
    }
    ImGui::SliderFloat("Sheet Transparency", &m_sheetTransparency, 0.0f, 1.0f);
    ImGui::SliderFloat("Spring Stiffness", &m_stiffness, 2.0f, 80.0f);
    ImGui::SliderFloat("Damping", &m_damping, 0.1f, 8.0f);
    ImGui::SliderFloat("Gravity", &m_gravity, 0.0f, 3.0f);
    ImGui::SliderFloat("Wind X", &m_windX, -2.0f, 2.0f);
    ImGui::SliderFloat("Wind Y", &m_windY, -2.0f, 2.0f);
    if (ImGui::Button("Reset Cloth"))
    {
        m_resetPending = true;
    }
    ImGui::TextUnformatted("Left mouse: place a new cloth sheet");
}

void Overlay_Cloth::processTopographyOverlay(
    const IntermediateData & data,
    TopographyProcessor & processor)
{
    if (data.topography.empty() || data.topography.type() != CV_32F)
    {
        return;
    }

    m_processor = &processor;
    if (m_topographySize.width > 0 && m_topographySize.height > 0
        && m_topographySize != data.topography.size())
    {
        m_center.x *= (float)data.topography.cols / m_topographySize.width;
        m_center.y *= (float)data.topography.rows / m_topographySize.height;
        m_resetPending = true;
    }
    m_topography = data.topography;
    m_topographySize = data.topography.size();

    if (m_resetPending)
    {
        createCloth();
    }
    updateCloth(data.deltaTime);
}

void Overlay_Cloth::renderOverlay(
    sf::RenderWindow & window,
    TopographyProcessor & processor)
{
    m_processor = &processor;
    renderCloth(window, processor);
}

void Overlay_Cloth::processOverlayEvent(
    const sf::Event & event,
    const sf::Vector2f & mouse,
    TopographyProcessor & processor)
{
    m_processor = &processor;
    const bool draggingProjection = processor.projector().processEvent(event, mouse);
    const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>();
    if (!mousePressed
        || mousePressed->button != sf::Mouse::Button::Left
        || draggingProjection || ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    cv::Point2f terrainPosition;
    if (mapMouseToTerrain(mouse, processor, terrainPosition))
    {
        m_center = terrainPosition;
        m_centerSet = true;
        m_resetPending = true;
    }
}

void Overlay_Cloth::saveOverlay(Settings & save) const
{
    Settings::json & settings = save.section("Overlay_Cloth");
    settings["m_cellsX"] = m_cellsX;
    settings["m_cellsY"] = m_cellsY;
    settings["m_clothSize"] = m_clothSize;
    settings["m_sheetTransparency"] = m_sheetTransparency;
    settings["m_stiffness"] = m_stiffness;
    settings["m_damping"] = m_damping;
    settings["m_gravity"] = m_gravity;
    settings["m_windX"] = m_windX;
    settings["m_windY"] = m_windY;
}

void Overlay_Cloth::loadOverlay(const Settings & save)
{
    const Settings::json & settings = save.section("Overlay_Cloth");
    Settings::read(settings, "m_cellsX", m_cellsX);
    Settings::read(settings, "m_cellsY", m_cellsY);
    Settings::read(settings, "m_clothSize", m_clothSize);
    Settings::read(settings, "m_sheetTransparency", m_sheetTransparency);
    Settings::read(settings, "m_stiffness", m_stiffness);
    Settings::read(settings, "m_damping", m_damping);
    Settings::read(settings, "m_gravity", m_gravity);
    Settings::read(settings, "m_windX", m_windX);
    Settings::read(settings, "m_windY", m_windY);
    m_resetPending = true;
}
