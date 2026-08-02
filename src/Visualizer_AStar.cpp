#include "Visualizer_AStar.h"

#include "Profiler.hpp"
#include "Timer.hpp"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
    constexpr float DiagonalDistance = 1.41421356237f;
    constexpr float SlopeScale = 100.0f;
    const std::array<cv::Point, 8> Directions = {
        cv::Point(-1, -1), cv::Point(0, -1), cv::Point(1, -1),
        cv::Point(-1, 0),                       cv::Point(1, 0),
        cv::Point(-1, 1),  cv::Point(0, 1),  cv::Point(1, 1)
    };

    float OctileDistance(const cv::Point & first, const cv::Point & second)
    {
        const int dx = std::abs(first.x - second.x);
        const int dy = std::abs(first.y - second.y);
        const int diagonal = std::min(dx, dy);
        const int straight = std::max(dx, dy) - diagonal;
        return diagonal * DiagonalDistance + (float)straight;
    }

    void AppendThickSegment(
        sf::VertexArray & vertices,
        const sf::Vector2f & first,
        const sf::Vector2f & second,
        float thickness,
        const sf::Color & color)
    {
        const sf::Vector2f difference = second - first;
        const float length = std::sqrt(
            difference.x * difference.x + difference.y * difference.y);
        if (length <= 0.0001f)
        {
            return;
        }

        const sf::Vector2f offset(
            -difference.y / length * thickness * 0.5f,
            difference.x / length * thickness * 0.5f);
        const sf::Vector2f firstLeft = first + offset;
        const sf::Vector2f firstRight = first - offset;
        const sf::Vector2f secondLeft = second + offset;
        const sf::Vector2f secondRight = second - offset;
        vertices.append(sf::Vertex(firstLeft, color));
        vertices.append(sf::Vertex(firstRight, color));
        vertices.append(sf::Vertex(secondLeft, color));
        vertices.append(sf::Vertex(firstRight, color));
        vertices.append(sf::Vertex(secondRight, color));
        vertices.append(sf::Vertex(secondLeft, color));
    }
}

bool Visualizer_AStar::isTraversable(int x, int y) const
{
    if (m_topography.empty() || m_topography.type() != CV_32F
        || x < 0 || y < 0 || x >= m_topography.cols || y >= m_topography.rows)
    {
        return false;
    }

    const float height = m_topography.at<float>(y, x);
    return std::isfinite(height) && height > 0.001f && height < 0.999f;
}

bool Visualizer_AStar::mapMouseToTerrain(
    const sf::Vector2f & mouse,
    cv::Point & terrainPoint) const
{
    if (m_topography.empty())
    {
        return false;
    }

    SandboxProjector & projector = this->projector();
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

    const sf::Vector2f local =
        (mouse - projector.getTransformedPosition()) / scale;
    std::vector<cv::Point2f> point = { { local.x, local.y } };
    cv::perspectiveTransform(point, point, inverseProjection);
    terrainPoint = {
        (int)std::round(point.front().x),
        (int)std::round(point.front().y) };
    return isTraversable(terrainPoint.x, terrainPoint.y);
}

void Visualizer_AStar::precalculateSlopes()
{
    const int width = m_topography.cols;
    const int height = m_topography.rows;
    const float unavailable = std::numeric_limits<float>::quiet_NaN();
    m_directionalSlopes.resize((size_t)width * height * Directions.size());
    m_traversableCells.resize((size_t)width * height);

    const float straightScale = SlopeScale / 10.0f;
    const float diagonalScale = straightScale / DiagonalDistance;

    for (int y = 0; y < height; y++)
    {
        const int previousY2 = std::max(y - 2, 0);
        const int previousY1 = std::max(y - 1, 0);
        const int nextY1 = std::min(y + 1, height - 1);
        const int nextY2 = std::min(y + 2, height - 1);
        const float * previousRow2 = m_topography.ptr<float>(previousY2);
        const float * previousRow1 = m_topography.ptr<float>(previousY1);
        const float * currentRow = m_topography.ptr<float>(y);
        const float * nextRow1 = m_topography.ptr<float>(nextY1);
        const float * nextRow2 = m_topography.ptr<float>(nextY2);
        for (int x = 0; x < width; x++)
        {
            const size_t cellIndex = (size_t)y * width + x;
            const size_t slopeIndex = cellIndex * Directions.size();
            const float terrainHeight = currentRow[x];
            const bool traversable = std::isfinite(terrainHeight)
                && terrainHeight > 0.001f && terrainHeight < 0.999f;
            m_traversableCells[cellIndex] = traversable;
            if (!traversable)
            {
                std::fill_n(
                    m_directionalSlopes.data() + slopeIndex,
                    Directions.size(),
                    unavailable);
                continue;
            }

            const int previousX2 = std::max(x - 2, 0);
            const int previousX1 = std::max(x - 1, 0);
            const int nextX1 = std::min(x + 1, width - 1);
            const int nextX2 = std::min(x + 2, width - 1);
            const float horizontal = (
                -2.0f * currentRow[previousX2]
                - currentRow[previousX1]
                + currentRow[nextX1]
                + 2.0f * currentRow[nextX2]) * straightScale;
            const float vertical = (
                -2.0f * previousRow2[x]
                - previousRow1[x]
                + nextRow1[x]
                + 2.0f * nextRow2[x]) * straightScale;
            const float downDiagonal = (
                -2.0f * previousRow2[previousX2]
                - previousRow1[previousX1]
                + nextRow1[nextX1]
                + 2.0f * nextRow2[nextX2]) * diagonalScale;
            const float upDiagonal = (
                -2.0f * nextRow2[previousX2]
                - nextRow1[previousX1]
                + previousRow1[nextX1]
                + 2.0f * previousRow2[nextX2]) * diagonalScale;

            m_directionalSlopes[slopeIndex + 0] = -downDiagonal;
            m_directionalSlopes[slopeIndex + 1] = -vertical;
            m_directionalSlopes[slopeIndex + 2] = upDiagonal;
            m_directionalSlopes[slopeIndex + 3] = -horizontal;
            m_directionalSlopes[slopeIndex + 4] = horizontal;
            m_directionalSlopes[slopeIndex + 5] = -upDiagonal;
            m_directionalSlopes[slopeIndex + 6] = vertical;
            m_directionalSlopes[slopeIndex + 7] = downDiagonal;
        }
    }
}

void Visualizer_AStar::calculatePath()
{
    m_path.clear();
    m_pathDistance = 0.0f;
    m_pathCost = 0.0f;
    m_searchTimeMilliseconds = 0.0f;
    m_nodesExpanded = 0;
    m_openListSize = 0;
    m_closedListSize = 0;
    m_pathFound = false;
    if (!m_hasStart || !m_hasGoal
        || !isTraversable(m_start.x, m_start.y)
        || !isTraversable(m_goal.x, m_goal.y))
    {
        return;
    }

    Timer searchTimer;
    const auto recordSearchTime = [&]()
    {
        m_searchTimeMilliseconds = searchTimer.elapsed() / 1000.0f;
    };

    const int width = m_topography.cols;
    const int height = m_topography.rows;
    const int nodeCount = width * height;
    const int startIndex = m_start.y * width + m_start.x;
    precalculateSlopes();
    const auto traversable = [&](int x, int y)
    {
        return x >= 0 && y >= 0 && x < width && y < height
            && m_traversableCells[(size_t)y * width + x] != 0;
    };
    const auto reachedGoal = [&](const cv::Point & point)
    {
        const int dx = point.x - m_goal.x;
        const int dy = point.y - m_goal.y;
        return dx * dx + dy * dy
            <= m_movementLength * m_movementLength;
    };
    const auto heuristic = [&](const cv::Point & point)
    {
        return std::max(
            0.0f,
            OctileDistance(point, m_goal)
                - m_movementLength * DiagonalDistance);
    };
    const float infinity = std::numeric_limits<float>::infinity();
    m_costFromStart.assign((size_t)nodeCount, infinity);
    m_estimatedCost.assign((size_t)nodeCount, infinity);
    m_cameFrom.assign((size_t)nodeCount, -1);
    m_heapPositions.assign((size_t)nodeCount, -1);
    m_closed.assign((size_t)nodeCount, false);
    m_openHeap.clear();
    m_openHeap.reserve((size_t)nodeCount);

    const auto comesBefore = [&](int leftIndex, int rightIndex)
    {
        const float leftEstimate = m_estimatedCost[(size_t)leftIndex];
        const float rightEstimate = m_estimatedCost[(size_t)rightIndex];
        if (leftEstimate != rightEstimate)
        {
            return leftEstimate < rightEstimate;
        }
        return m_costFromStart[(size_t)leftIndex]
            > m_costFromStart[(size_t)rightIndex];
    };
    const auto swapHeapEntries = [&](size_t first, size_t second)
    {
        std::swap(m_openHeap[first], m_openHeap[second]);
        m_heapPositions[(size_t)m_openHeap[first]] = (int)first;
        m_heapPositions[(size_t)m_openHeap[second]] = (int)second;
    };
    const auto siftUp = [&](size_t position)
    {
        while (position > 0)
        {
            const size_t parent = (position - 1) / 2;
            if (!comesBefore(m_openHeap[position], m_openHeap[parent]))
            {
                break;
            }
            swapHeapEntries(position, parent);
            position = parent;
        }
    };
    const auto siftDown = [&](size_t position)
    {
        while (true)
        {
            const size_t left = position * 2 + 1;
            if (left >= m_openHeap.size())
            {
                break;
            }
            const size_t right = left + 1;
            size_t best = left;
            if (right < m_openHeap.size()
                && comesBefore(m_openHeap[right], m_openHeap[left]))
            {
                best = right;
            }
            if (!comesBefore(m_openHeap[best], m_openHeap[position]))
            {
                break;
            }
            swapHeapEntries(position, best);
            position = best;
        }
    };
    const auto pushOrDecrease = [&](int nodeIndex, float estimatedCost)
    {
        m_estimatedCost[(size_t)nodeIndex] = estimatedCost;
        int & position = m_heapPositions[(size_t)nodeIndex];
        if (position < 0)
        {
            position = (int)m_openHeap.size();
            m_openHeap.push_back(nodeIndex);
        }
        siftUp((size_t)position);
    };
    const auto popMinimum = [&]()
    {
        const int result = m_openHeap.front();
        const int last = m_openHeap.back();
        m_openHeap.pop_back();
        m_heapPositions[(size_t)result] = -1;
        if (!m_openHeap.empty())
        {
            m_openHeap.front() = last;
            m_heapPositions[(size_t)last] = 0;
            siftDown(0);
        }
        return result;
    };

    m_costFromStart[(size_t)startIndex] = 0.0f;
    pushOrDecrease(startIndex, heuristic(m_start));

    int reachedIndex = -1;
    while (!m_openHeap.empty())
    {
        const int currentIndex = popMinimum();
        if (m_closed[(size_t)currentIndex])
        {
            continue;
        }
        m_closed[(size_t)currentIndex] = true;
        m_closedListSize++;
        const cv::Point current(currentIndex % width, currentIndex / width);
        if (reachedGoal(current))
        {
            reachedIndex = currentIndex;
            break;
        }
        m_nodesExpanded++;

        for (size_t directionIndex = 0;
            directionIndex < Directions.size();
            directionIndex++)
        {
            const cv::Point & direction = Directions[directionIndex];
            const bool diagonal = direction.x != 0 && direction.y != 0;
            const float distance = diagonal ? DiagonalDistance : 1.0f;
            cv::Point next = current;
            float actionCost = 0.0f;
            bool legalAction = true;
            for (int movement = 0; movement < m_movementLength; movement++)
            {
                const cv::Point previous = next;
                next += direction;
                if (!traversable(next.x, next.y)
                    || (diagonal
                        && (!traversable(previous.x + direction.x, previous.y)
                            || !traversable(previous.x, previous.y + direction.y))))
                {
                    legalAction = false;
                    break;
                }

                const int previousIndex = previous.y * width + previous.x;
                const float signedSlope = m_directionalSlopes[
                    (size_t)previousIndex * Directions.size() + directionIndex];
                if (!std::isfinite(signedSlope)
                    || std::abs(signedSlope) > m_maximumLegalSlope)
                {
                    legalAction = false;
                    break;
                }

                const float slope = std::abs(signedSlope);
                const float slopePenalty = signedSlope >= 0.0f
                    ? m_uphillPenalty
                    : m_downhillPenalty;
                actionCost += distance
                    * (1.0f + slope * slope * slopePenalty);
            }
            if (!legalAction)
            {
                continue;
            }

            const int nextIndex = next.y * width + next.x;
            if (m_closed[(size_t)nextIndex])
            {
                continue;
            }

            const float candidateCost =
                m_costFromStart[(size_t)currentIndex] + actionCost;
            if (candidateCost >= m_costFromStart[(size_t)nextIndex])
            {
                continue;
            }

            m_costFromStart[(size_t)nextIndex] = candidateCost;
            m_cameFrom[(size_t)nextIndex] = currentIndex;
            pushOrDecrease(
                nextIndex,
                candidateCost + heuristic(next));
        }
    }
    m_openListSize = (int)m_openHeap.size();

    if (reachedIndex < 0)
    {
        recordSearchTime();
        return;
    }

    for (int index = reachedIndex; index >= 0; index = m_cameFrom[(size_t)index])
    {
        m_path.push_back({ index % width, index / width });
        if (index == startIndex)
        {
            break;
        }
    }
    if (m_path.empty() || m_path.back() != m_start)
    {
        m_path.clear();
        recordSearchTime();
        return;
    }

    std::reverse(m_path.begin(), m_path.end());
    for (size_t index = 1; index < m_path.size(); index++)
    {
        const cv::Point difference = m_path[index] - m_path[index - 1];
        m_pathDistance += std::sqrt(
            (float)(difference.x * difference.x + difference.y * difference.y));
    }
    m_pathCost = m_costFromStart[(size_t)reachedIndex];
    m_pathFound = true;
    recordSearchTime();
}

void Visualizer_AStar::drawPath(
    sf::RenderWindow & window) const
{
    if (!m_hasStart)
    {
        return;
    }

    std::vector<cv::Point2f> terrainPoints;
    terrainPoints.reserve(m_path.size() + 2);
    if (m_pathFound)
    {
        for (const cv::Point & point : m_path)
        {
            terrainPoints.push_back({ (float)point.x, (float)point.y });
        }
    }
    const size_t startPointIndex = terrainPoints.size();
    terrainPoints.push_back({ (float)m_start.x, (float)m_start.y });
    const size_t goalPointIndex = terrainPoints.size();
    if (m_hasGoal)
    {
        terrainPoints.push_back({ (float)m_goal.x, (float)m_goal.y });
    }

    const cv::Mat projection = projector().getProjectionMatrix();
    const float scale = projector().getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }
    cv::perspectiveTransform(terrainPoints, terrainPoints, projection);

    const sf::Vector2f origin = projector().getTransformedPosition();
    const auto screenPoint = [&](size_t index)
    {
        return sf::Vector2f(
            origin.x + terrainPoints[index].x * scale,
            origin.y + terrainPoints[index].y * scale);
    };

    if (m_pathFound && m_path.size() >= 2)
    {
        sf::VertexArray outline(sf::PrimitiveType::Triangles);
        sf::VertexArray line(sf::PrimitiveType::Triangles);
        for (size_t index = 1; index < m_path.size(); index++)
        {
            const sf::Vector2f first = screenPoint(index - 1);
            const sf::Vector2f second = screenPoint(index);
            AppendThickSegment(
                outline,
                first,
                second,
                m_pathThickness + 3.0f,
                sf::Color(18, 20, 24, 210));
            AppendThickSegment(
                line,
                first,
                second,
                m_pathThickness,
                sf::Color(255, 214, 48, 245));
        }
        window.draw(outline, sf::BlendAlpha);
        window.draw(line, sf::BlendAlpha);
    }

    const auto drawMarker = [&](size_t index, const sf::Color & color)
    {
        const float radius = std::max(6.0f, m_pathThickness * 1.15f);
        sf::CircleShape marker(radius, 28);
        marker.setOrigin({ radius, radius });
        marker.setPosition(screenPoint(index));
        marker.setFillColor(color);
        marker.setOutlineColor(sf::Color(20, 22, 26, 235));
        marker.setOutlineThickness(std::max(2.0f, m_pathThickness * 0.25f));
        window.draw(marker, sf::BlendAlpha);
    };
    drawMarker(startPointIndex, sf::Color(52, 220, 92, 245));
    if (m_hasGoal)
    {
        drawMarker(goalPointIndex, sf::Color(235, 62, 58, 245));
    }
}

void Visualizer_AStar::clearPoints()
{
    m_path.clear();
    m_pathDistance = 0.0f;
    m_pathCost = 0.0f;
    m_searchTimeMilliseconds = 0.0f;
    m_nodesExpanded = 0;
    m_openListSize = 0;
    m_closedListSize = 0;
    m_hasStart = false;
    m_hasGoal = false;
    m_nextPointIsStart = true;
    m_pathFound = false;
}

void Visualizer_AStar::init()
{
    clearPoints();
}

void Visualizer_AStar::imgui()
{
    PROFILE_FUNCTION();

    ImGui::SliderFloat(
        "Uphill Slope Penalty",
        &m_uphillPenalty,
        0.0f,
        500.0f,
        "%.1f");
    ImGui::SliderFloat(
        "Downhill Slope Penalty",
        &m_downhillPenalty,
        0.0f,
        500.0f,
        "%.1f");
    ImGui::SliderFloat(
        "Maximum Legal Slope",
        &m_maximumLegalSlope,
        0.0f,
        6.25f,
        "%.2f%% / cell");
    ImGui::SliderInt("Movement Length", &m_movementLength, 1, 32);
    ImGui::SliderFloat(
        "Path Thickness",
        &m_pathThickness,
        1.0f,
        30.0f,
        "%.1f px");

    ImGui::Separator();
    ImGui::Text("Nodes Expanded: %d", m_nodesExpanded);
    ImGui::Text("Open List Size: %d", m_openListSize);
    ImGui::Text("Closed List Size: %d", m_closedListSize);
    ImGui::Text("Search Time: %.2f ms", m_searchTimeMilliseconds);
    ImGui::Separator();

    if (!m_hasStart)
    {
        ImGui::TextUnformatted("Left mouse: set start point");
    }
    else if (!m_hasGoal)
    {
        ImGui::TextUnformatted("Left mouse: set goal point");
    }
    else
    {
        ImGui::TextUnformatted("Left mouse: begin a new path");
        if (m_pathFound)
        {
            ImGui::Text("Path Distance: %.1f px", m_pathDistance);
            ImGui::Text("Weighted Cost: %.1f", m_pathCost);
        }
        else
        {
            ImGui::TextUnformatted("No traversable path found");
        }
    }

    if (ImGui::Button("Clear Points"))
    {
        clearPoints();
    }
}

void Visualizer_AStar::process(
    const TerrainFrame & data)
{
    PROFILE_FUNCTION();

    if (data.heightMap.empty() || data.heightMap.type() != CV_32F)
    {
        m_topography.release();
        m_path.clear();
        m_pathFound = false;
        return;
    }

    if (m_topographySize.width > 0 && m_topographySize.height > 0
        && m_topographySize != data.heightMap.size())
    {
        const float xScale = (float)data.heightMap.cols / m_topographySize.width;
        const float yScale = (float)data.heightMap.rows / m_topographySize.height;
        if (m_hasStart)
        {
            m_start.x = std::clamp(
                (int)std::round(m_start.x * xScale),
                0,
                data.heightMap.cols - 1);
            m_start.y = std::clamp(
                (int)std::round(m_start.y * yScale),
                0,
                data.heightMap.rows - 1);
        }
        if (m_hasGoal)
        {
            m_goal.x = std::clamp(
                (int)std::round(m_goal.x * xScale),
                0,
                data.heightMap.cols - 1);
            m_goal.y = std::clamp(
                (int)std::round(m_goal.y * yScale),
                0,
                data.heightMap.rows - 1);
        }
    }

    m_topography = data.heightMap;
    m_topographySize = data.heightMap.size();
    calculatePath();
}

void Visualizer_AStar::render(
    sf::RenderWindow & window)
{
    PROFILE_FUNCTION();
    drawPath(window);
}

void Visualizer_AStar::processEvent(
    const sf::Event & event,
    const sf::Vector2f & mouse)
{
    const auto * mousePressed = event.getIf<sf::Event::MouseButtonPressed>();
    if (!mousePressed || mousePressed->button != sf::Mouse::Button::Left
        || ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    cv::Point point;
    if (!mapMouseToTerrain(mouse, point))
    {
        return;
    }

    if (m_nextPointIsStart)
    {
        m_start = point;
        m_hasStart = true;
        m_hasGoal = false;
        m_path.clear();
        m_pathFound = false;
    }
    else
    {
        m_goal = point;
        m_hasGoal = true;
    }
    m_nextPointIsStart = !m_nextPointIsStart;
    calculatePath();
}

void Visualizer_AStar::save(Settings & save) const
{
    Settings::json & settings = save.section("Visualizer_AStar");
    settings["m_uphillPenalty"] = m_uphillPenalty;
    settings["m_downhillPenalty"] = m_downhillPenalty;
    settings["m_maximumLegalSlope"] = m_maximumLegalSlope;
    settings["m_movementLength"] = m_movementLength;
    settings["m_pathThickness"] = m_pathThickness;
}

void Visualizer_AStar::load(const Settings & save)
{
    const Settings::json & settings = save.section("Visualizer_AStar");
    Settings::read(settings, "m_uphillPenalty", m_uphillPenalty);
    Settings::read(settings, "m_downhillPenalty", m_downhillPenalty);
    Settings::read(settings, "m_maximumLegalSlope", m_maximumLegalSlope);
    Settings::read(settings, "m_movementLength", m_movementLength);
    Settings::read(settings, "m_pathThickness", m_pathThickness);
    m_uphillPenalty = std::clamp(m_uphillPenalty, 0.0f, 500.0f);
    m_downhillPenalty = std::clamp(m_downhillPenalty, 0.0f, 500.0f);
    m_maximumLegalSlope = std::clamp(m_maximumLegalSlope, 0.0f, 6.25f);
    m_movementLength = std::clamp(m_movementLength, 1, 32);
    m_pathThickness = std::clamp(m_pathThickness, 1.0f, 30.0f);
    clearPoints();
}
