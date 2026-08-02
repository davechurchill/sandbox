#include "DataWarper.h"
#include "Profiler.hpp"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <opencv2/imgproc.hpp>
#include <utility>

namespace
{
    constexpr float HandleRadius = 10.0f;
    constexpr float MinimumEdgeLength = 1.0f;
    constexpr int MaximumOutputDimension = 4096;
    constexpr std::int64_t MaximumOutputPixels =
        static_cast<std::int64_t>(MaximumOutputDimension) * MaximumOutputDimension;

    bool IsFinite(const cv::Point2f & point)
    {
        return std::isfinite(point.x) && std::isfinite(point.y);
    }

    template <std::size_t PointCount>
    int ClickedPointIndex(const sf::Vector2f & mouse, const std::array<cv::Point2f, PointCount> & points)
    {
        const float radiusSquared = HandleRadius * HandleRadius;
        for (std::size_t index = 0; index < points.size(); ++index)
        {
            const float dx = mouse.x - points[index].x;
            const float dy = mouse.y - points[index].y;
            if (dx * dx + dy * dy <= radiusSquared)
            {
                return static_cast<int>(index);
            }
        }
        return -1;
    }

    bool SampleMedianDepth(const cv::Mat & matrix, const cv::Point2f & point, float & result)
    {
        // A single depth-camera pixel can be missing or noisy. Use the median of
        // the valid values in a 5x5 neighborhood so one bad reading cannot tilt
        // the entire calibration plane.
        constexpr int SampleRadius = 2;
        std::array<float, 25> samples{};
        std::size_t sampleCount = 0;
        const int centerX = static_cast<int>(point.x);
        const int centerY = static_cast<int>(point.y);

        for (int y = std::max(0, centerY - SampleRadius); y <= std::min(matrix.rows - 1, centerY + SampleRadius); ++y)
        {
            const float * row = matrix.ptr<float>(y);
            for (int x = std::max(0, centerX - SampleRadius); x <= std::min(matrix.cols - 1, centerX + SampleRadius); ++x)
            {
                const float value = row[x];
                if (std::isfinite(value) && value > 0.0f)
                {
                    samples[sampleCount++] = value;
                }
            }
        }

        if (sampleCount == 0) { return false; }
        const auto middle = samples.begin() + sampleCount / 2;
        std::nth_element(samples.begin(), middle, samples.begin() + sampleCount);
        result = *middle;
        return true;
    }
}

DataWarper::DataWarper()
{
    m_warpCircle = sf::CircleShape(HandleRadius, 64);
    m_warpCircle.setOrigin({ HandleRadius, HandleRadius });
    m_warpCircle.setFillColor(sf::Color::Green);

    m_planarCircle = sf::CircleShape(HandleRadius, 64);
    m_planarCircle.setOrigin({ HandleRadius, HandleRadius });
    m_planarCircle.setFillColor(sf::Color::Cyan);

    generateWarpMatrix();
}

void DataWarper::imgui()
{
    PROFILE_FUNCTION();
 
    if (ImGui::Checkbox("Draw Camera Region", &m_drawCameraRegion) && !m_drawCameraRegion)
    {
        m_dragWarpPoint = -1;
    }

    if (ImGui::Checkbox("Apply Height Adjust", &m_applyHeightAdjustment))
    {
        m_dragPlanarPoint = -1;
        if (m_applyHeightAdjustment)
        {
            m_updatePlane = true;
        }
    }
    if (ImGui::Button("Update Height Adjustment"))
    {
        m_updatePlane = true;
    }
    ImGui::Text("Plane Norm: [%f, %f, %f]", m_plane[0], m_plane[1], m_plane[2]);

    if (ImGui::SliderFloat("Data Size", &m_dataSize, 0.1f, 2.0f))
    {
        generateWarpMatrix();
    }

    ImGui::Text("Dimensions: (%d, %d)", m_width, m_height);
}

void DataWarper::transformRect(const cv::Mat& input, cv::Mat& output)
{
    const bool pointsInBounds = std::all_of(
        m_warpPoints.begin(),
        m_warpPoints.end(),
        [&input](const cv::Point2f & point)
        {
            return point.x >= 0.0f && point.x < input.cols
                && point.y >= 0.0f && point.y < input.rows;
        });
    if (input.empty() || m_warpMatrix.empty() || !pointsInBounds)
    {
        output.release();
        return;
    }

    // m_warpMatrix is a homography: it maps the selected camera quadrilateral
    // into the rectangular output while applying the required perspective divide.
    cv::warpPerspective(input, output, m_warpMatrix, cv::Size(m_width, m_height));
}

void DataWarper::heightAdjustment(cv::Mat & matrix)
{
    PROFILE_FUNCTION();

    if (m_planeValid && m_planeSize != matrix.size())
    {
        m_planeValid = false;
        m_updatePlane = true;
        m_heightOffsets.release();
    }

    if (m_updatePlane)
    {
        const auto pointIsInBounds = [&matrix](const cv::Point2f & point)
        {
            return point.x >= 0 && point.x < matrix.cols && point.y >= 0 && point.y < matrix.rows;
        };

        if (matrix.empty() || matrix.type() != CV_32F
            || !pointIsInBounds(m_planarPoints[0])
            || !pointIsInBounds(m_planarPoints[1])
            || !pointIsInBounds(m_planarPoints[2]))
        {
            m_planeValid = false;
            m_heightOffsets.release();
            return;
        }

        float firstPointZ = 0.0f;
        float thirdPointZ = 0.0f;
        if (!SampleMedianDepth(matrix, m_planarPoints[0], firstPointZ)
            || !SampleMedianDepth(matrix, m_planarPoints[1], m_baseHeight)
            || !SampleMedianDepth(matrix, m_planarPoints[2], thirdPointZ))
        {
            m_planeValid = false;
            m_heightOffsets.release();
            return;
        }

        // Treat each calibration sample as a 3D point (image x, image y, depth z).
        // These are two vectors lying along the measured sandbox surface, both
        // starting at calibration point 0.
        float vect_A[] = { m_planarPoints[1].x - m_planarPoints[0].x, m_planarPoints[1].y - m_planarPoints[0].y, m_baseHeight - firstPointZ };
        float vect_B[] = { m_planarPoints[2].x - m_planarPoints[0].x, m_planarPoints[2].y - m_planarPoints[0].y,   thirdPointZ - firstPointZ };

        // The cross product A x B is perpendicular to the surface. Its three
        // components become A, B and C in the plane equation:
        //
        //     A*x + B*y + C*z + D = 0
        m_plane[0] = vect_A[1] * vect_B[2] - vect_A[2] * vect_B[1];
        m_plane[1] = vect_A[2] * vect_B[0] - vect_A[0] * vect_B[2];
        m_plane[2] = vect_A[0] * vect_B[1] - vect_A[1] * vect_B[0];

        // C must be nonzero because the correction solves the equation for z.
        // A near-zero C describes a nearly vertical plane in image/depth space.
        if (std::abs(m_plane[2]) < 1e-6f)
        {
            m_planeValid = false;
            m_heightOffsets.release();
            return;
        }

        // Substitute calibration point 1 into the equation to solve for D.
        m_plane[3] = -(m_plane[0] * m_planarPoints[1].x + m_plane[1] * m_planarPoints[1].y + m_plane[2] * m_baseHeight);
        m_updatePlane = false;
        m_planeValid = true;
        m_planeSize = matrix.size();
        generateHeightOffsets(matrix.size());
    }

    if (!m_planeValid || matrix.empty() || matrix.type() != CV_32F) { return; }
    if (m_heightOffsets.size() != matrix.size())
    {
        generateHeightOffsets(matrix.size());
    }
    if (m_heightOffsets.empty()) { return; }

    for (int y = 0; y < matrix.rows; ++y)
    {
        float * values = matrix.ptr<float>(y);
        const float * offsets = m_heightOffsets.ptr<float>(y);
        for (int x = 0; x < matrix.cols; ++x)
        {
            // Nonpositive or nonfinite values represent missing camera depth and
            // must remain untouched rather than becoming artificial terrain.
            if (std::isfinite(values[x]) && values[x] > 0.0f)
            {
                values[x] += offsets[x];
            }
        }
    }
}

void DataWarper::generateHeightOffsets(cv::Size size)
{
    if (!m_planeValid || size.width <= 0 || size.height <= 0)
    {
        m_heightOffsets.release();
        return;
    }

    // The fitted plane does not change every frame, so cache one correction per
    // pixel. Solving A*x + B*y + C*z + D = 0 for z gives:
    //
    //     planeZ = (-D - A*x - B*y) / C
    //
    // Adding (baseHeight - planeZ) makes the sampled plane uniformly equal to
    // baseHeight while retaining terrain deviations above and below that plane.
    m_heightOffsets.create(size, CV_32F);
    const float inverseZ = 1.0f / m_plane[2];
    for (int y = 0; y < size.height; ++y)
    {
        float * offsets = m_heightOffsets.ptr<float>(y);
        const float rowPlane = -m_plane[3] - m_plane[1] * y;
        for (int x = 0; x < size.width; ++x)
        {
            const float planeHeight = (rowPlane - m_plane[0] * x) * inverseZ;
            offsets[x] = m_baseHeight - planeHeight;
        }
    }
}

void DataWarper::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    PROFILE_FUNCTION();

    // detect if we have clicked a circle
    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>();
        mousePressed && mousePressed->button == sf::Mouse::Button::Left)
    {
        m_dragWarpPoint = -1;
        m_dragPlanarPoint = m_applyHeightAdjustment
            ? ClickedPointIndex(mouse, m_planarPoints)
            : -1;
        if (m_dragPlanarPoint == -1 && m_drawCameraRegion)
        {
            m_dragWarpPoint = ClickedPointIndex(mouse, m_warpPoints);
        }
    }

    // if we have released the mouse button
    if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>();
        mouseReleased && mouseReleased->button == sf::Mouse::Button::Left)
    {
        m_dragWarpPoint = -1;
        m_dragPlanarPoint = -1;
    }

    // if the mouse moved and we are dragging something, update its position and regenerate the matrix
    if (event.is<sf::Event::MouseMoved>())
    {
        if (m_drawCameraRegion && m_dragWarpPoint != -1)
        {
            m_warpPoints[m_dragWarpPoint] = { mouse.x, mouse.y };
            generateWarpMatrix();
        }

        if (m_applyHeightAdjustment && m_dragPlanarPoint != -1)
        {
            m_planarPoints[m_dragPlanarPoint] = { mouse.x, mouse.y };
            m_planeValid = false;
            m_heightOffsets.release();
            m_updatePlane = m_applyHeightAdjustment;
        }
    }
}

void DataWarper::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (m_drawCameraRegion)
    {
        // draw the circles and lines used to calibrate the interior of the sandbox for the depth camera
        for (const cv::Point2f & point : m_warpPoints)
        {
            m_warpCircle.setPosition({ point.x, point.y });
            window.draw(m_warpCircle);
        }

        sf::VertexArray boxInteriorVertices(sf::PrimitiveType::LineStrip);
        boxInteriorVertices.append(sf::Vertex({ m_warpPoints[0].x, m_warpPoints[0].y }));
        boxInteriorVertices.append(sf::Vertex({ m_warpPoints[1].x, m_warpPoints[1].y }));
        boxInteriorVertices.append(sf::Vertex({ m_warpPoints[3].x, m_warpPoints[3].y }));
        boxInteriorVertices.append(sf::Vertex({ m_warpPoints[2].x, m_warpPoints[2].y }));
        boxInteriorVertices.append(sf::Vertex({ m_warpPoints[0].x, m_warpPoints[0].y }));
        window.draw(boxInteriorVertices);
    }

    if (m_applyHeightAdjustment)
    {
        // draw the circles and lines used to calibrate the height adjustment
        for (const cv::Point2f & point : m_planarPoints)
        {
            m_planarCircle.setPosition({ point.x, point.y });
            window.draw(m_planarCircle);
        }

        sf::VertexArray planarVertices(sf::PrimitiveType::LineStrip);
        planarVertices.append(sf::Vertex({ m_planarPoints[0].x, m_planarPoints[0].y }));
        planarVertices.append(sf::Vertex({ m_planarPoints[1].x, m_planarPoints[1].y }));
        planarVertices.append(sf::Vertex({ m_planarPoints[2].x, m_planarPoints[2].y }));
        planarVertices.append(sf::Vertex({ m_planarPoints[0].x, m_planarPoints[0].y }));
        window.draw(planarVertices);
    }
}

bool DataWarper::transformPoints(const std::vector<cv::Point2f> & input, std::vector<cv::Point2f> & output) const
{
    if (input.empty() || m_warpMatrix.empty())
    {
        output.clear();
        return false;
    }
    cv::perspectiveTransform(input, output, m_warpMatrix);
    return cv::checkRange(output);
}

void DataWarper::generateWarpMatrix()
{
    PROFILE_FUNCTION();

    m_warpMatrix.release();
    m_width = 0;
    m_height = 0;

    if (!std::isfinite(m_dataSize)) { m_dataSize = 1.0f; }
    m_dataSize = std::clamp(m_dataSize, 0.1f, 2.0f);
    if (!std::all_of(m_warpPoints.begin(), m_warpPoints.end(), IsFinite)) { return; }

    // The transform stores bottom-left before bottom-right, but polygon tests
    // must walk around the perimeter: TL -> TR -> BR -> BL.
    std::vector<cv::Point2f> corners = { m_warpPoints[0], m_warpPoints[1], m_warpPoints[3], m_warpPoints[2] };
    if (!cv::isContourConvex(corners) || std::abs(cv::contourArea(corners)) < 1.0)
    {
        return;
    }

    // A camera view of a rectangle is normally a trapezoid. Use the longer of
    // each pair of opposite edges so perspective correction does not discard
    // resolution from the wider side. Data Size scales the resulting rectangle.
    const float topWidth = static_cast<float>(cv::norm(m_warpPoints[1] - m_warpPoints[0]));
    const float bottomWidth = static_cast<float>(cv::norm(m_warpPoints[3] - m_warpPoints[2]));
    const float leftHeight = static_cast<float>(cv::norm(m_warpPoints[2] - m_warpPoints[0]));
    const float rightHeight = static_cast<float>(cv::norm(m_warpPoints[3] - m_warpPoints[1]));
    const float width = std::max(topWidth, bottomWidth) * m_dataSize;
    const float height = std::max(leftHeight, rightHeight) * m_dataSize;
    if (width < MinimumEdgeLength || height < MinimumEdgeLength
        || width > MaximumOutputDimension || height > MaximumOutputDimension)
    {
        return;
    }

    const int outputWidth = static_cast<int>(std::round(width));
    const int outputHeight = static_cast<int>(std::round(height));
    if (outputWidth < 2 || outputHeight < 2
        || static_cast<std::int64_t>(outputWidth) * outputHeight > MaximumOutputPixels)
    {
        return;
    }

    m_width = outputWidth;
    m_height = outputHeight;

    // Map TL, TR, BL and BR to the matching corners of the output image. Pixel
    // coordinates end at width-1 and height-1, not width and height.
    const std::array<cv::Point2f, 4> destinationPoints = {
            cv::Point2f(0.0f, 0.0f),
            cv::Point2f(static_cast<float>(m_width - 1), 0.0f),
            cv::Point2f(0.0f, static_cast<float>(m_height - 1)),
            cv::Point2f(static_cast<float>(m_width - 1), static_cast<float>(m_height - 1)),
    };
    // OpenCV solves the eight independent coefficients of the 3x3 homography
    // from these four source/destination point pairs.
    cv::Mat warpMatrix = cv::getPerspectiveTransform(m_warpPoints.data(), destinationPoints.data());
    if (!cv::checkRange(warpMatrix))
    {
        m_width = 0;
        m_height = 0;
        return;
    }
    m_warpMatrix = std::move(warpMatrix);
}

void DataWarper::save(Settings & save) const
{
    Settings::json & settings = save.section("DataWarper");
    settings["m_warpPoints"] = Settings::json::array();
    for (const cv::Point2f & point : m_warpPoints)
    {
        settings["m_warpPoints"].push_back({ point.x, point.y });
    }
    settings["m_applyHeightAdjustment"] = m_applyHeightAdjustment;
    settings["m_dataSize"] = m_dataSize;
    settings["m_drawCameraRegion"] = m_drawCameraRegion;
    settings["m_planarPoints"] = Settings::json::array();
    for (const cv::Point2f & point : m_planarPoints)
    {
        settings["m_planarPoints"].push_back({ point.x, point.y });
    }
}
void DataWarper::load(const Settings & save)
{
    const Settings::json & settings = save.section("DataWarper");
    const auto readPointArray = [&settings](const char * key, cv::Point2f * points, size_t count)
    {
        const auto found = settings.find(key);
        if (found == settings.end() || !found->is_array() || found->size() != count)
        {
            return;
        }
        for (size_t index = 0; index < count; index++)
        {
            const Settings::json & point = found->at(index);
            if (point.is_array() && point.size() == 2
                && point[0].is_number() && point[1].is_number())
            {
                const cv::Point2f loadedPoint{ point[0].get<float>(), point[1].get<float>() };
                if (IsFinite(loadedPoint))
                {
                    points[index] = loadedPoint;
                }
            }
        }
    };
    readPointArray("m_warpPoints", m_warpPoints.data(), m_warpPoints.size());
    Settings::read(settings, "m_applyHeightAdjustment", m_applyHeightAdjustment);
    const auto dataSize = settings.find("m_dataSize");
    if (dataSize != settings.end() && dataSize->is_number())
    {
        const float loadedDataSize = dataSize->get<float>();
        if (std::isfinite(loadedDataSize))
        {
            m_dataSize = std::clamp(loadedDataSize, 0.1f, 2.0f);
        }
    }
    Settings::read(settings, "m_drawCameraRegion", m_drawCameraRegion);
    readPointArray("m_planarPoints", m_planarPoints.data(), m_planarPoints.size());
    m_planeValid = false;
    m_heightOffsets.release();
    m_planeSize = {};
    m_updatePlane = m_applyHeightAdjustment;
    generateWarpMatrix();
}
