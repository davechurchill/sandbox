#include "Terrain3DView.h"

#include <SFML/OpenGL.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
    constexpr float Pi = 3.14159265358979323846f;

    float Clamp01(float value)
    {
        if (!std::isfinite(value))
        {
            return 0.0f;
        }
        return std::clamp(value, 0.0f, 1.0f);
    }

    float SampleHeight(const cv::Mat & heightMap, int x, int y)
    {
        x = std::clamp(x, 0, heightMap.cols - 1);
        y = std::clamp(y, 0, heightMap.rows - 1);
        return heightMap.ptr<float>(y)[x];
    }

    sf::Vector3f Cross(const sf::Vector3f & left, const sf::Vector3f & right)
    {
        return {
            left.y * right.z - left.z * right.y,
            left.z * right.x - left.x * right.z,
            left.x * right.y - left.y * right.x
        };
    }

    float Dot(const sf::Vector3f & left, const sf::Vector3f & right)
    {
        return left.x * right.x + left.y * right.y + left.z * right.z;
    }

    sf::Vector3f Normalize(const sf::Vector3f & value)
    {
        const float lengthSquared = Dot(value, value);
        if (lengthSquared <= 0.0000001f)
        {
            return { 0.0f, 1.0f, 0.0f };
        }
        return value / std::sqrt(lengthSquared);
    }

    sf::Vector3f Mix(
        const sf::Vector3f & start,
        const sf::Vector3f & end,
        float amount)
    {
        return start + (end - start) * Clamp01(amount);
    }

    sf::Vector3f HeightColor(float height)
    {
        height = Clamp01(height);
        if (height < 0.22f)
        {
            return Mix(
                { 0.035f, 0.12f, 0.24f },
                { 0.05f, 0.34f, 0.42f },
                height / 0.22f);
        }
        if (height < 0.32f)
        {
            return Mix(
                { 0.05f, 0.34f, 0.42f },
                { 0.32f, 0.47f, 0.18f },
                (height - 0.22f) / 0.10f);
        }
        if (height < 0.67f)
        {
            return Mix(
                { 0.32f, 0.47f, 0.18f },
                { 0.42f, 0.56f, 0.22f },
                (height - 0.32f) / 0.35f);
        }
        if (height < 0.86f)
        {
            return Mix(
                { 0.42f, 0.56f, 0.22f },
                { 0.45f, 0.43f, 0.40f },
                (height - 0.67f) / 0.19f);
        }
        return Mix(
            { 0.45f, 0.43f, 0.40f },
            { 0.88f, 0.90f, 0.91f },
            (height - 0.86f) / 0.14f);
    }

    void ApplyPerspective(float verticalFieldOfView, float aspect, float nearPlane, float farPlane)
    {
        const float top = nearPlane * std::tan(verticalFieldOfView * Pi / 360.0f);
        const float right = top * aspect;
        glFrustum(-right, right, -top, top, nearPlane, farPlane);
    }

    void ApplyLookAt(
        const sf::Vector3f & eye,
        const sf::Vector3f & target,
        const sf::Vector3f & worldUp)
    {
        const sf::Vector3f forward = Normalize(target - eye);
        const sf::Vector3f side = Normalize(Cross(forward, worldUp));
        const sf::Vector3f up = Cross(side, forward);
        const GLfloat orientation[16] = {
            side.x, up.x, -forward.x, 0.0f,
            side.y, up.y, -forward.y, 0.0f,
            side.z, up.z, -forward.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        glMultMatrixf(orientation);
        glTranslatef(-eye.x, -eye.y, -eye.z);
    }
}

void Terrain3DView::open()
{
    sf::ContextSettings contextSettings;
    contextSettings.depthBits = 24;
    contextSettings.antiAliasingLevel = 4;
    contextSettings.majorVersion = 2;
    contextSettings.minorVersion = 1;

    m_window.create(
        sf::VideoMode({ WindowWidth, WindowHeight }),
        "3D Terrain - Left drag: orbit | Right/middle drag: pan | Wheel: zoom | R: reset",
        sf::Style::Default,
        sf::State::Windowed,
        contextSettings);
    m_window.setVerticalSyncEnabled(true);
    resetCamera();

    if (!m_window.setActive())
    {
        std::cerr << "Failed to activate the 3D terrain window OpenGL context.\n";
        m_window.close();
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);
    if (!m_window.setActive(false))
    {
        std::cerr << "Failed to release the 3D terrain window OpenGL context.\n";
    }
}

void Terrain3DView::toggle()
{
    if (m_window.isOpen())
    {
        close();
    }
    else
    {
        open();
    }
}

void Terrain3DView::close()
{
    m_orbiting = false;
    m_panning = false;
    m_hasVisualizerTexture = false;
    m_visualizationCaptured = false;
    m_window.close();
}

bool Terrain3DView::isOpen() const
{
    return m_window.isOpen();
}

void Terrain3DView::captureVisualization(
    const cv::Mat & heightMap,
    const sf::RenderWindow & target,
    const cv::Mat & projectionMatrix,
    const sf::Vector2f & projectedPosition,
    float projectedScale)
{
    m_visualizationCaptured = false;
    if (!m_window.isOpen()
        || !target.isOpen()
        || heightMap.empty()
        || heightMap.type() != CV_32FC1
        || projectionMatrix.rows != 3
        || projectionMatrix.cols != 3
        || !std::isfinite(projectedPosition.x)
        || !std::isfinite(projectedPosition.y)
        || !std::isfinite(projectedScale)
        || projectedScale <= 0.0f)
    {
        m_hasVisualizerTexture = false;
        return;
    }

    const sf::Vector2u targetSize = target.getSize();
    if (targetSize.x == 0 || targetSize.y == 0)
    {
        m_hasVisualizerTexture = false;
        return;
    }

    if (m_visualizerTexture.getSize() != targetSize)
    {
        if (!m_visualizerTexture.resize(targetSize))
        {
            std::cerr << "Failed to create the 3D terrain visualizer texture.\n";
            m_hasVisualizerTexture = false;
            return;
        }
        m_visualizerTexture.setSmooth(true);
    }
    m_visualizerTexture.update(target);

    const int meshColumns = std::min(heightMap.cols, MaximumMeshResolution);
    const int meshRows = std::min(heightMap.rows, MaximumMeshResolution);
    const std::size_t vertexCount = (std::size_t)meshColumns * meshRows;
    std::vector<cv::Point2f> terrainPoints;
    terrainPoints.reserve(vertexCount);
    for (int y = 0; y < meshRows; ++y)
    {
        const float sourceY = meshRows > 1
            ? (float)y * (heightMap.rows - 1) / (meshRows - 1)
            : 0.0f;
        for (int x = 0; x < meshColumns; ++x)
        {
            const float sourceX = meshColumns > 1
                ? (float)x * (heightMap.cols - 1) / (meshColumns - 1)
                : 0.0f;
            terrainPoints.emplace_back(sourceX, sourceY);
        }
    }

    std::vector<cv::Point2f> projectedPoints;
    try
    {
        cv::perspectiveTransform(
            terrainPoints,
            projectedPoints,
            projectionMatrix);
    }
    catch (const cv::Exception &)
    {
        m_hasVisualizerTexture = false;
        return;
    }
    if (projectedPoints.size() != vertexCount)
    {
        m_hasVisualizerTexture = false;
        return;
    }

    m_textureCoordinates.resize(vertexCount * 2);
    for (std::size_t index = 0; index < vertexCount; ++index)
    {
        const cv::Point2f & projected = projectedPoints[index];
        if (!std::isfinite(projected.x) || !std::isfinite(projected.y))
        {
            m_textureCoordinates.clear();
            m_hasVisualizerTexture = false;
            return;
        }

        const sf::Vector2f worldPosition = projectedPosition + sf::Vector2f{
            projected.x * projectedScale,
            projected.y * projectedScale
        };
        const sf::Vector2i pixel = target.mapCoordsToPixel(worldPosition);
        m_textureCoordinates[index * 2] = (float)pixel.x;
        m_textureCoordinates[index * 2 + 1] = (float)pixel.y;
    }

    m_hasVisualizerTexture = true;
    m_visualizationCaptured = true;
}

void Terrain3DView::resetCamera()
{
    m_yaw = -0.75f;
    m_pitch = 0.65f;
    m_distance = 1.75f;
    m_target = { 0.0f, 0.0f, 0.0f };
    m_orbiting = false;
    m_panning = false;
}

void Terrain3DView::processEvent(const sf::Event & event)
{
    if (event.is<sf::Event::Closed>())
    {
        close();
        return;
    }

    if (event.is<sf::Event::FocusLost>())
    {
        m_orbiting = false;
        m_panning = false;
        return;
    }

    if (const auto * key = event.getIf<sf::Event::KeyPressed>())
    {
        if (key->code == sf::Keyboard::Key::Escape)
        {
            close();
        }
        else if (key->code == sf::Keyboard::Key::R)
        {
            resetCamera();
        }
        return;
    }

    if (const auto * pressed = event.getIf<sf::Event::MouseButtonPressed>())
    {
        m_previousMousePosition = pressed->position;
        if (pressed->button == sf::Mouse::Button::Left)
        {
            m_orbiting = true;
        }
        else if (pressed->button == sf::Mouse::Button::Right
            || pressed->button == sf::Mouse::Button::Middle)
        {
            m_panning = true;
        }
        return;
    }

    if (const auto * released = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (released->button == sf::Mouse::Button::Left)
        {
            m_orbiting = false;
        }
        else if (released->button == sf::Mouse::Button::Right
            || released->button == sf::Mouse::Button::Middle)
        {
            m_panning = false;
        }
        return;
    }

    if (const auto * wheel = event.getIf<sf::Event::MouseWheelScrolled>())
    {
        m_distance *= std::pow(0.84f, wheel->delta);
        m_distance = std::clamp(m_distance, 0.18f, 8.0f);
        return;
    }

    const auto * moved = event.getIf<sf::Event::MouseMoved>();
    if (!moved)
    {
        return;
    }

    const sf::Vector2i delta = moved->position - m_previousMousePosition;
    m_previousMousePosition = moved->position;

    if (m_orbiting)
    {
        m_yaw -= delta.x * 0.007f;
        m_pitch += delta.y * 0.007f;
        m_pitch = std::clamp(m_pitch, 0.04f, 1.50f);
    }
    if (m_panning)
    {
        const float cosPitch = std::cos(m_pitch);
        const sf::Vector3f eye = m_target + sf::Vector3f{
            m_distance * cosPitch * std::sin(m_yaw),
            m_distance * std::sin(m_pitch),
            m_distance * cosPitch * std::cos(m_yaw)
        };
        const sf::Vector3f forward = Normalize(m_target - eye);
        const sf::Vector3f right = Normalize(Cross(forward, { 0.0f, 1.0f, 0.0f }));
        const sf::Vector3f up = Normalize(Cross(right, forward));
        const float scale = m_distance * 0.0012f;
        m_target += right * (-delta.x * scale) + up * (delta.y * scale);
    }
}

void Terrain3DView::processEvents()
{
    while (const auto event = m_window.pollEvent())
    {
        processEvent(*event);
        if (!m_window.isOpen())
        {
            break;
        }
    }
}

void Terrain3DView::rebuildMesh(const cv::Mat & heightMap)
{
    m_sourceColumns = heightMap.cols;
    m_sourceRows = heightMap.rows;
    m_meshColumns = std::min(m_sourceColumns, MaximumMeshResolution);
    m_meshRows = std::min(m_sourceRows, MaximumMeshResolution);

    const float aspect = (float)m_sourceColumns / m_sourceRows;
    m_terrainWidth = aspect >= 1.0f ? aspect : 1.0f;
    m_terrainDepth = aspect >= 1.0f ? 1.0f : 1.0f / aspect;

    const std::size_t vertexCount = (std::size_t)m_meshColumns * m_meshRows;
    m_vertices.assign(vertexCount * 3, 0.0f);
    m_normals.assign(vertexCount * 3, 0.0f);
    m_colors.assign(vertexCount * 3, 0.0f);

    for (int y = 0; y < m_meshRows; ++y)
    {
        const float normalizedY = m_meshRows > 1
            ? (float)y / (m_meshRows - 1)
            : 0.0f;
        for (int x = 0; x < m_meshColumns; ++x)
        {
            const float normalizedX = m_meshColumns > 1
                ? (float)x / (m_meshColumns - 1)
                : 0.0f;
            const std::size_t offset = ((std::size_t)y * m_meshColumns + x) * 3;
            m_vertices[offset] = (normalizedX - 0.5f) * m_terrainWidth;
            m_vertices[offset + 2] = (normalizedY - 0.5f) * m_terrainDepth;
        }
    }

    m_indices.clear();
    if (m_meshColumns > 1 && m_meshRows > 1)
    {
        m_indices.reserve(
            (std::size_t)(m_meshColumns - 1)
            * (m_meshRows - 1)
            * 6);
        for (int y = 0; y < m_meshRows - 1; ++y)
        {
            for (int x = 0; x < m_meshColumns - 1; ++x)
            {
                const unsigned int topLeft = (unsigned int)(y * m_meshColumns + x);
                const unsigned int topRight = topLeft + 1;
                const unsigned int bottomLeft = topLeft + m_meshColumns;
                const unsigned int bottomRight = bottomLeft + 1;
                m_indices.push_back(topLeft);
                m_indices.push_back(bottomLeft);
                m_indices.push_back(topRight);
                m_indices.push_back(topRight);
                m_indices.push_back(bottomLeft);
                m_indices.push_back(bottomRight);
            }
        }
    }
}

void Terrain3DView::updateNormals()
{
    for (int y = 0; y < m_meshRows; ++y)
    {
        const int previousY = std::max(y - 1, 0);
        const int nextY = std::min(y + 1, m_meshRows - 1);
        for (int x = 0; x < m_meshColumns; ++x)
        {
            const int previousX = std::max(x - 1, 0);
            const int nextX = std::min(x + 1, m_meshColumns - 1);
            const std::size_t leftOffset = ((std::size_t)y * m_meshColumns + previousX) * 3;
            const std::size_t rightOffset = ((std::size_t)y * m_meshColumns + nextX) * 3;
            const std::size_t nearOffset = ((std::size_t)previousY * m_meshColumns + x) * 3;
            const std::size_t farOffset = ((std::size_t)nextY * m_meshColumns + x) * 3;
            const std::size_t offset = ((std::size_t)y * m_meshColumns + x) * 3;

            const sf::Vector3f across{
                m_vertices[rightOffset] - m_vertices[leftOffset],
                m_vertices[rightOffset + 1] - m_vertices[leftOffset + 1],
                m_vertices[rightOffset + 2] - m_vertices[leftOffset + 2]
            };
            const sf::Vector3f forward{
                m_vertices[farOffset] - m_vertices[nearOffset],
                m_vertices[farOffset + 1] - m_vertices[nearOffset + 1],
                m_vertices[farOffset + 2] - m_vertices[nearOffset + 2]
            };
            const sf::Vector3f normal = Normalize(Cross(forward, across));
            m_normals[offset] = normal.x;
            m_normals[offset + 1] = normal.y;
            m_normals[offset + 2] = normal.z;
        }
    }
}

void Terrain3DView::updateMesh(const cv::Mat & heightMap)
{
    if (heightMap.cols != m_sourceColumns
        || heightMap.rows != m_sourceRows
        || m_vertices.empty())
    {
        rebuildMesh(heightMap);
    }

    for (int y = 0; y < m_meshRows; ++y)
    {
        const int sourceY = m_meshRows > 1
            ? (int)std::lround((double)y * (heightMap.rows - 1) / (m_meshRows - 1))
            : 0;
        for (int x = 0; x < m_meshColumns; ++x)
        {
            const int sourceX = m_meshColumns > 1
                ? (int)std::lround((double)x * (heightMap.cols - 1) / (m_meshColumns - 1))
                : 0;
            const float height = Clamp01(SampleHeight(heightMap, sourceX, sourceY));
            const std::size_t offset = ((std::size_t)y * m_meshColumns + x) * 3;
            m_vertices[offset + 1] = (height - 0.5f) * m_heightScale;

            const sf::Vector3f color = HeightColor(height);
            m_colors[offset] = color.x;
            m_colors[offset + 1] = color.y;
            m_colors[offset + 2] = color.z;
        }
    }
    updateNormals();
}

void Terrain3DView::render()
{
    if (!m_window.setActive())
    {
        return;
    }

    const sf::Vector2u windowSize = m_window.getSize();
    if (windowSize.x == 0 || windowSize.y == 0)
    {
        if (!m_window.setActive(false))
        {
            std::cerr << "Failed to release the 3D terrain window OpenGL context.\n";
        }
        return;
    }

    glViewport(0, 0, (GLsizei)windowSize.x, (GLsizei)windowSize.y);
    glClearColor(0.012f, 0.016f, 0.025f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    ApplyPerspective(45.0f, (float)windowSize.x / windowSize.y, 0.02f, 50.0f);

    const float cosPitch = std::cos(m_pitch);
    const sf::Vector3f eye = m_target + sf::Vector3f{
        m_distance * cosPitch * std::sin(m_yaw),
        m_distance * std::sin(m_pitch),
        m_distance * cosPitch * std::cos(m_yaw)
    };
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    ApplyLookAt(eye, m_target, { 0.0f, 1.0f, 0.0f });

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    const GLfloat ambient[] = { 0.34f, 0.36f, 0.40f, 1.0f };
    const GLfloat diffuse[] = { 0.95f, 0.92f, 0.84f, 1.0f };
    const GLfloat specular[] = { 0.22f, 0.22f, 0.20f, 1.0f };
    const GLfloat lightDirection[] = { -0.65f, 1.0f, 0.45f, 0.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightDirection);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 24.0f);

    if (!m_indices.empty())
    {
        const bool useVisualizerTexture = m_hasVisualizerTexture
            && m_visualizerTexture.getSize().x > 0
            && m_visualizerTexture.getSize().y > 0
            && m_textureCoordinates.size() == m_vertices.size() / 3 * 2;

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, m_vertices.data());
        glNormalPointer(GL_FLOAT, 0, m_normals.data());
        if (useVisualizerTexture)
        {
            glColor3f(1.0f, 1.0f, 1.0f);
            glEnable(GL_TEXTURE_2D);
            sf::Texture::bind(
                &m_visualizerTexture,
                sf::CoordinateType::Pixels);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(2, GL_FLOAT, 0, m_textureCoordinates.data());
        }
        else
        {
            glEnableClientState(GL_COLOR_ARRAY);
            glColorPointer(3, GL_FLOAT, 0, m_colors.data());
        }
        glDrawElements(
            GL_TRIANGLES,
            (GLsizei)m_indices.size(),
            GL_UNSIGNED_INT,
            m_indices.data());
        if (useVisualizerTexture)
        {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            sf::Texture::bind(nullptr);
            glDisable(GL_TEXTURE_2D);
        }
        else
        {
            glDisableClientState(GL_COLOR_ARRAY);
        }
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
    m_window.display();
    if (!m_window.setActive(false))
    {
        std::cerr << "Failed to release the 3D terrain window OpenGL context.\n";
    }
}

void Terrain3DView::update(const cv::Mat & heightMap)
{
    if (!m_window.isOpen())
    {
        return;
    }

    processEvents();
    if (!m_window.isOpen())
    {
        return;
    }

    if (!m_visualizationCaptured)
    {
        m_hasVisualizerTexture = false;
    }

    if (!heightMap.empty() && heightMap.type() == CV_32FC1)
    {
        updateMesh(heightMap);
    }
    else
    {
        m_vertices.clear();
        m_normals.clear();
        m_colors.clear();
        m_textureCoordinates.clear();
        m_indices.clear();
        m_hasVisualizerTexture = false;
        m_sourceColumns = 0;
        m_sourceRows = 0;
        m_meshColumns = 0;
        m_meshRows = 0;
    }
    render();
    m_visualizationCaptured = false;
}
