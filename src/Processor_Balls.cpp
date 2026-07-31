#include "Processor_Balls.h"
#include "Profiler.hpp"
#include "Tools.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
    constexpr const char * BallsShaderPath = "shaders/shader_balls.frag";
    constexpr const char * BallSphereShaderPath = "shaders/shader_ball_sphere.frag";
    constexpr float RadiansToDegrees = 57.29577951308232f;
    constexpr size_t MaxBallTrails = 400;

    bool isTerrainCell(float height)
    {
        return std::isfinite(height) && height > 0.001f && height < 0.999f;
    }
}

void Processor_Balls::init()
{
    reloadShader();
}

SandBoxProjector & Processor_Balls::activeProjector()
{
    return m_overlayProcessor ? m_overlayProcessor->projector() : m_projector;
}

void Processor_Balls::reloadShader()
{
    m_shaderLoaded = m_shader.loadFromFile(BallsShaderPath, sf::Shader::Fragment);
    if (m_shaderLoaded)
    {
        m_shader.setUniform("currentTexture", sf::Shader::CurrentTexture);
    }
    m_ballShaderLoaded = m_ballShader.loadFromFile(
        BallSphereShaderPath,
        sf::Shader::Fragment);
}

void Processor_Balls::resetBalls()
{
    m_balls.clear();
    m_trails.clear();
    m_defaultBallCreated = true;
    m_randomResetPending = true;
}

sf::Color Processor_Balls::randomBallColor()
{
    std::uniform_real_distribution<float> hueDistribution(0.0f, 360.0f);
    std::uniform_real_distribution<float> saturationDistribution(0.62f, 0.88f);
    std::uniform_real_distribution<float> valueDistribution(0.78f, 0.96f);
    const float hue = hueDistribution(m_random) / 60.0f;
    const float saturation = saturationDistribution(m_random);
    const float value = valueDistribution(m_random);
    const int sector = (int)std::floor(hue) % 6;
    const float fraction = hue - std::floor(hue);
    const float p = value * (1.0f - saturation);
    const float q = value * (1.0f - fraction * saturation);
    const float t = value * (1.0f - (1.0f - fraction) * saturation);

    float red = value;
    float green = t;
    float blue = p;
    switch (sector)
    {
    case 1: red = q; green = value; blue = p; break;
    case 2: red = p; green = value; blue = t; break;
    case 3: red = p; green = q; blue = value; break;
    case 4: red = t; green = p; blue = value; break;
    case 5: red = value; green = p; blue = q; break;
    }

    return sf::Color(
        (sf::Uint8)std::round(red * 255.0f),
        (sf::Uint8)std::round(green * 255.0f),
        (sf::Uint8)std::round(blue * 255.0f));
}

void Processor_Balls::imgui()
{
    PROFILE_FUNCTION();

    ImGui::Text("Balls: %d", (int)m_balls.size());
    ImGui::SliderFloat("Gravity", &m_gravity, 0.0f, 5000.0f, "%.0f");
    ImGui::SliderFloat("Ball Speed Multiplier", &m_ballSpeedMultiplier, 0.1f, 4.0f, "%.1fx");
    ImGui::SliderFloat("Rolling Resistance", &m_rollingResistance, 0.0f, 4.0f);
    ImGui::SliderFloat("Ball Size", &m_ballSize, 6.0f, 40.0f);
    ImGui::SliderFloat("Ball Bounciness", &m_ballRestitution, 0.0f, 1.0f);
    if (ImGui::Checkbox("Lava Appearance", &m_lavaAppearance)
        && m_lavaAppearance && m_trailLength <= 0.0f)
    {
        m_trailLength = 3.0f;
    }
    ImGui::SliderFloat("Trail Length", &m_trailLength, 0.0f, 10.0f, "%.1f sec");

    ImGui::TextUnformatted("Left mouse: add ball");

    if (ImGui::Button("Reset Balls"))
    {
        resetBalls();
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload Shader"))
    {
        reloadShader();
    }

    ImGui::Separator();
    m_projector.imgui();
}

bool Processor_Balls::isValidTerrainPosition(
    const cv::Mat & terrain,
    const cv::Point2f & position) const
{
    if (terrain.empty() || position.x < 0.0f || position.y < 0.0f
        || position.x >= terrain.cols || position.y >= terrain.rows)
    {
        return false;
    }

    const int x = std::clamp((int)std::round(position.x), 0, terrain.cols - 1);
    const int y = std::clamp((int)std::round(position.y), 0, terrain.rows - 1);
    return isTerrainCell(terrain.at<float>(y, x));
}

bool Processor_Balls::sampleTerrainHeight(
    const cv::Mat & terrain,
    const cv::Point2f & position,
    float & height) const
{
    if (terrain.empty() || position.x < 0.0f || position.y < 0.0f
        || position.x > terrain.cols - 1 || position.y > terrain.rows - 1)
    {
        return false;
    }

    const int x0 = std::clamp((int)std::floor(position.x), 0, terrain.cols - 1);
    const int y0 = std::clamp((int)std::floor(position.y), 0, terrain.rows - 1);
    const int x1 = std::min(x0 + 1, terrain.cols - 1);
    const int y1 = std::min(y0 + 1, terrain.rows - 1);
    const float h00 = terrain.at<float>(y0, x0);
    const float h10 = terrain.at<float>(y0, x1);
    const float h01 = terrain.at<float>(y1, x0);
    const float h11 = terrain.at<float>(y1, x1);
    if (!isTerrainCell(h00) || !isTerrainCell(h10)
        || !isTerrainCell(h01) || !isTerrainCell(h11))
    {
        return false;
    }

    const float xAmount = position.x - x0;
    const float yAmount = position.y - y0;
    const float top = h00 + (h10 - h00) * xAmount;
    const float bottom = h01 + (h11 - h01) * xAmount;
    height = top + (bottom - top) * yAmount;
    return true;
}

bool Processor_Balls::addBall(const cv::Point2f & position)
{
    if (!isValidTerrainPosition(m_topography, position))
    {
        return false;
    }

    Ball ball;
    ball.position = position;
    if (!m_balls.empty())
    {
        ball.color = randomBallColor();
    }
    m_balls.push_back(ball);
    return true;
}

bool Processor_Balls::spawnBallNearMiddle(const cv::Mat & terrain)
{
    if (terrain.empty())
    {
        return false;
    }

    const cv::Point2f middle(terrain.cols * 0.5f, terrain.rows * 0.5f);
    float bestDistanceSquared = std::numeric_limits<float>::max();
    cv::Point2f bestPosition;
    bool found = false;
    for (int y = 0; y < terrain.rows; y++)
    {
        for (int x = 0; x < terrain.cols; x++)
        {
            const cv::Point2f position((float)x, (float)y);
            if (!isValidTerrainPosition(terrain, position))
            {
                continue;
            }

            const float dx = position.x - middle.x;
            const float dy = position.y - middle.y;
            const float distanceSquared = dx * dx + dy * dy;
            if (distanceSquared < bestDistanceSquared)
            {
                bestDistanceSquared = distanceSquared;
                bestPosition = position;
                found = true;
            }
        }
    }

    return found && addBall(bestPosition);
}

bool Processor_Balls::findRandomTerrainPosition(
    const cv::Mat & terrain,
    cv::Point2f & position)
{
    if (terrain.empty())
    {
        return false;
    }

    std::uniform_int_distribution<int> xDistribution(0, terrain.cols - 1);
    std::uniform_int_distribution<int> yDistribution(0, terrain.rows - 1);
    for (int attempt = 0; attempt < 1024; attempt++)
    {
        const cv::Point2f candidate((float)xDistribution(m_random), (float)yDistribution(m_random));
        if (isValidTerrainPosition(terrain, candidate))
        {
            position = candidate;
            return true;
        }
    }

    for (int y = 0; y < terrain.rows; y++)
    {
        for (int x = 0; x < terrain.cols; x++)
        {
            const cv::Point2f candidate((float)x, (float)y);
            if (isValidTerrainPosition(terrain, candidate))
            {
                position = candidate;
                return true;
            }
        }
    }

    return false;
}

bool Processor_Balls::spawnRandomBall(const cv::Mat & terrain)
{
    cv::Point2f position;
    return findRandomTerrainPosition(terrain, position) && addBall(position);
}

float Processor_Balls::getTerrainBallRadius(const cv::Mat & terrain) const
{
    const float sizeScale = m_ballSize / 18.0f;
    return std::max(1.0f, std::min(terrain.cols, terrain.rows) * 0.012f * sizeScale);
}

void Processor_Balls::resolveBallCollisions(const cv::Mat & terrain)
{
    const float visibleBallDiameter = m_ballSize * 1.10f;
    const float fallbackMinimumDistance = getTerrainBallRadius(terrain) * 2.0f * 1.10f;
    SandBoxProjector & projector = activeProjector();
    const cv::Mat projection = projector.getProjectionMatrix();
    const float projectorScale = projector.getTransformedScale();
    const bool useProjectedDistance = !projection.empty()
        && std::isfinite(projectorScale) && projectorScale > 0.0f;

    for (int iteration = 0; iteration < 2; iteration++)
    {
        std::vector<cv::Point2f> projectedPoints;
        if (useProjectedDistance)
        {
            projectedPoints.reserve(m_balls.size());
            for (const Ball & ball : m_balls)
            {
                projectedPoints.push_back(ball.position);
            }
            cv::perspectiveTransform(projectedPoints, projectedPoints, projection);
        }

        for (size_t firstIndex = 0; firstIndex < m_balls.size(); firstIndex++)
        {
            for (size_t secondIndex = firstIndex + 1; secondIndex < m_balls.size(); secondIndex++)
            {
                Ball & first = m_balls[firstIndex];
                Ball & second = m_balls[secondIndex];
                const float dx = second.position.x - first.position.x;
                const float dy = second.position.y - first.position.y;
                const float distanceSquared = dx * dx + dy * dy;

                float normalX = 1.0f;
                float normalY = 0.0f;
                float distance = 0.0f;
                if (distanceSquared > 0.000001f)
                {
                    distance = std::sqrt(distanceSquared);
                    normalX = dx / distance;
                    normalY = dy / distance;
                }
                else
                {
                    const float relativeX = second.velocity.x - first.velocity.x;
                    const float relativeY = second.velocity.y - first.velocity.y;
                    const float relativeLength = std::sqrt(relativeX * relativeX + relativeY * relativeY);
                    if (relativeLength > 0.0001f)
                    {
                        normalX = relativeX / relativeLength;
                        normalY = relativeY / relativeLength;
                    }
                }

                float minimumDistance = fallbackMinimumDistance;
                if (useProjectedDistance && distance > 0.0001f)
                {
                    const cv::Point2f & projectedFirst = projectedPoints[firstIndex];
                    const cv::Point2f & projectedSecond = projectedPoints[secondIndex];
                    const float projectedDx = (projectedSecond.x - projectedFirst.x) * projectorScale;
                    const float projectedDy = (projectedSecond.y - projectedFirst.y) * projectorScale;
                    const float projectedDistance = std::sqrt(
                        projectedDx * projectedDx + projectedDy * projectedDy);
                    if (std::isfinite(projectedDistance) && projectedDistance > 0.0001f)
                    {
                        if (projectedDistance >= visibleBallDiameter)
                        {
                            continue;
                        }
                        minimumDistance = visibleBallDiameter * distance / projectedDistance;
                    }
                }

                if (distance >= minimumDistance)
                {
                    continue;
                }

                const float overlap = std::max(minimumDistance - distance, 0.0f);
                const float correction = overlap * 0.5f;
                const cv::Point2f correctedFirst(
                    first.position.x - normalX * correction,
                    first.position.y - normalY * correction);
                const cv::Point2f correctedSecond(
                    second.position.x + normalX * correction,
                    second.position.y + normalY * correction);
                const bool firstCanMove = isValidTerrainPosition(terrain, correctedFirst);
                const bool secondCanMove = isValidTerrainPosition(terrain, correctedSecond);
                if (firstCanMove && secondCanMove)
                {
                    first.position = correctedFirst;
                    second.position = correctedSecond;
                }
                else if (firstCanMove)
                {
                    const cv::Point2f fullCorrection(
                        first.position.x - normalX * overlap,
                        first.position.y - normalY * overlap);
                    first.position = isValidTerrainPosition(terrain, fullCorrection)
                        ? fullCorrection
                        : correctedFirst;
                }
                else if (secondCanMove)
                {
                    const cv::Point2f fullCorrection(
                        second.position.x + normalX * overlap,
                        second.position.y + normalY * overlap);
                    second.position = isValidTerrainPosition(terrain, fullCorrection)
                        ? fullCorrection
                        : correctedSecond;
                }

                float relativeX = second.velocity.x - first.velocity.x;
                float relativeY = second.velocity.y - first.velocity.y;
                const float velocityAlongNormal = relativeX * normalX + relativeY * normalY;
                if (velocityAlongNormal >= 0.0f)
                {
                    continue;
                }

                const float impulseMagnitude = -(1.0f + m_ballRestitution) * velocityAlongNormal * 0.5f;
                const float impulseX = normalX * impulseMagnitude;
                const float impulseY = normalY * impulseMagnitude;
                first.velocity.x -= impulseX;
                first.velocity.y -= impulseY;
                second.velocity.x += impulseX;
                second.velocity.y += impulseY;

                relativeX = second.velocity.x - first.velocity.x;
                relativeY = second.velocity.y - first.velocity.y;
                const float normalSpeed = relativeX * normalX + relativeY * normalY;
                float tangentX = relativeX - normalX * normalSpeed;
                float tangentY = relativeY - normalY * normalSpeed;
                const float tangentLength = std::sqrt(tangentX * tangentX + tangentY * tangentY);
                if (tangentLength > 0.0001f)
                {
                    tangentX /= tangentLength;
                    tangentY /= tangentLength;
                    float frictionMagnitude = -(relativeX * tangentX + relativeY * tangentY) * 0.5f;
                    const float maximumFriction = impulseMagnitude * 0.04f;
                    frictionMagnitude = std::clamp(frictionMagnitude, -maximumFriction, maximumFriction);
                    const float frictionX = tangentX * frictionMagnitude;
                    const float frictionY = tangentY * frictionMagnitude;
                    first.velocity.x -= frictionX;
                    first.velocity.y -= frictionY;
                    second.velocity.x += frictionX;
                    second.velocity.y += frictionY;
                }
            }
        }
    }
}

void Processor_Balls::updateBalls(const cv::Mat & terrain, float deltaTime)
{
    if (m_randomResetPending)
    {
        if (!spawnRandomBall(terrain))
        {
            return;
        }
        m_randomResetPending = false;
    }

    if (!m_defaultBallCreated)
    {
        if (!spawnBallNearMiddle(terrain))
        {
            return;
        }
        m_defaultBallCreated = true;
    }

    const float dt = std::clamp(deltaTime, 0.0f, 0.05f);
    const float terrainRadius = getTerrainBallRadius(terrain);
    for (Ball & ball : m_balls)
    {
        if (!isValidTerrainPosition(terrain, ball.position))
        {
            cv::Point2f position;
            if (findRandomTerrainPosition(terrain, position))
            {
                const sf::Color color = ball.color;
                ball = Ball{};
                ball.position = position;
                ball.color = color;
            }
            continue;
        }

        float currentHeight = 0.0f;
        if (!sampleTerrainHeight(terrain, ball.position, currentHeight))
        {
            currentHeight = terrain.at<float>(
                std::clamp((int)std::round(ball.position.y), 0, terrain.rows - 1),
                std::clamp((int)std::round(ball.position.x), 0, terrain.cols - 1));
        }

        constexpr float sampleDistance = 2.0f;
        float leftHeight = currentHeight;
        float rightHeight = currentHeight;
        float upHeight = currentHeight;
        float downHeight = currentHeight;
        sampleTerrainHeight(terrain, { ball.position.x - sampleDistance, ball.position.y }, leftHeight);
        sampleTerrainHeight(terrain, { ball.position.x + sampleDistance, ball.position.y }, rightHeight);
        sampleTerrainHeight(terrain, { ball.position.x, ball.position.y - sampleDistance }, upHeight);
        sampleTerrainHeight(terrain, { ball.position.x, ball.position.y + sampleDistance }, downHeight);

        const float gradientX = (rightHeight - leftHeight) / (sampleDistance * 2.0f);
        const float gradientY = (downHeight - upHeight) / (sampleDistance * 2.0f);
        ball.velocity.x -= gradientX * m_gravity * m_ballSpeedMultiplier * dt;
        ball.velocity.y -= gradientY * m_gravity * m_ballSpeedMultiplier * dt;

        const float damping = std::exp(-m_rollingResistance * dt);
        ball.velocity.x *= damping;
        ball.velocity.y *= damping;

        const float speed = std::sqrt(
            ball.velocity.x * ball.velocity.x
            + ball.velocity.y * ball.velocity.y);
        const float terrainMaximumSpeed = std::min(terrain.cols, terrain.rows)
            * 0.40f * m_ballSpeedMultiplier;
        const float collisionSafeSpeed = terrainRadius * 0.8f / std::max(dt, 0.001f);
        const float maximumSpeed = std::min(terrainMaximumSpeed, collisionSafeSpeed);
        if (speed > maximumSpeed && speed > 0.0f)
        {
            const float speedScale = maximumSpeed / speed;
            ball.velocity.x *= speedScale;
            ball.velocity.y *= speedScale;
        }

        cv::Point2f nextPosition = ball.position;
        const cv::Point2f nextX(ball.position.x + ball.velocity.x * dt, ball.position.y);
        if (isValidTerrainPosition(terrain, nextX))
        {
            nextPosition.x = nextX.x;
        }
        else
        {
            ball.velocity.x *= -0.35f;
        }

        const cv::Point2f nextY(nextPosition.x, ball.position.y + ball.velocity.y * dt);
        if (isValidTerrainPosition(terrain, nextY))
        {
            nextPosition.y = nextY.y;
        }
        else
        {
            ball.velocity.y *= -0.35f;
        }

        ball.position = nextPosition;
    }

    resolveBallCollisions(terrain);

    for (Ball & ball : m_balls)
    {
        const float finalSpeed = std::sqrt(
            ball.velocity.x * ball.velocity.x
            + ball.velocity.y * ball.velocity.y);
        ball.rotation = std::fmod(
            ball.rotation + finalSpeed * dt / terrainRadius * RadiansToDegrees,
            360.0f);
        ball.trailTimer += dt;
        if (m_trailLength > 0.0f && finalSpeed > 0.15f && ball.trailTimer >= 0.05f)
        {
            m_trails.push_back({
                ball.position,
                m_lavaAppearance ? sf::Color(255, 78, 8) : ball.color,
                m_trailLength,
                m_trailLength,
                m_lavaAppearance });
            ball.trailTimer = 0.0f;
        }
    }

    for (BallTrail & trail : m_trails)
    {
        trail.remaining -= dt;
    }
    std::erase_if(m_trails, [](const BallTrail & trail) { return trail.remaining <= 0.0f; });
    if (m_trailLength <= 0.0f)
    {
        m_trails.clear();
    }
    else if (m_trails.size() > MaxBallTrails)
    {
        m_trails.erase(
            m_trails.begin(),
            m_trails.begin() + (m_trails.size() - MaxBallTrails));
    }
}

void Processor_Balls::drawBall(
    sf::RenderWindow & window,
    const sf::Vector2f & position,
    const sf::Vector2f & direction,
    const sf::Color & color,
    float rotation,
    float visualScale,
    float movementAmount)
{
    const float radius = m_ballSize * 0.5f * visualScale;
    const float directionLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    const sf::Vector2f forward = directionLength > 0.001f
        ? direction / directionLength
        : sf::Vector2f(1.0f, 0.0f);
    const float heading = std::atan2(forward.y, forward.x) * RadiansToDegrees;

    sf::CircleShape shadow(radius, 32);
    shadow.setOrigin(radius, radius);
    shadow.setPosition(position.x + radius * 0.32f, position.y + radius * 0.42f);
    shadow.setScale(1.18f, 0.72f);
    shadow.setFillColor(sf::Color(0, 0, 0, 105));
    window.draw(shadow);

    if (m_ballShaderLoaded)
    {
        if (m_lavaAppearance)
        {
            const float glowRadius = radius * 1.55f;
            sf::CircleShape glow(glowRadius, 36);
            glow.setOrigin(glowRadius, glowRadius);
            glow.setPosition(position);
            glow.setFillColor(sf::Color(255, 45, 0, 68));
            window.draw(glow, sf::BlendAdd);
        }

        sf::CircleShape shadedBall(radius, 64);
        shadedBall.setOrigin(radius, radius);
        shadedBall.setPosition(position);
        shadedBall.setFillColor(sf::Color::White);

        const sf::Vector2i centerPixel = window.mapCoordsToPixel(position);
        const sf::Vector2i radiusPixel = window.mapCoordsToPixel(
            position + sf::Vector2f(radius, 0.0f));
        const float shaderRadius = std::max(1.0f, std::sqrt(
            (float)((radiusPixel.x - centerPixel.x) * (radiusPixel.x - centerPixel.x)
                + (radiusPixel.y - centerPixel.y) * (radiusPixel.y - centerPixel.y))));
        m_ballShader.setUniform("ballCenter", sf::Glsl::Vec2(
            (float)centerPixel.x,
            (float)window.getSize().y - centerPixel.y));
        m_ballShader.setUniform("ballRadius", shaderRadius);
        m_ballShader.setUniform("baseColor", sf::Glsl::Vec4(
            color.r / 255.0f,
            color.g / 255.0f,
            color.b / 255.0f,
            1.0f));
        m_ballShader.setUniform("rotation", rotation);
        m_ballShader.setUniform("movementDirection", sf::Glsl::Vec2(
            forward.x,
            -forward.y));
        m_ballShader.setUniform("movementAmount", movementAmount);
        m_ballShader.setUniform("lavaMode", m_lavaAppearance ? 1 : 0);
        static sf::Clock shaderClock;
        m_ballShader.setUniform("u_time", shaderClock.getElapsedTime().asSeconds());
        window.draw(shadedBall, &m_ballShader);
        return;
    }

    if (m_lavaAppearance)
    {
        const float glowRadius = radius * 1.55f;
        sf::CircleShape glow(glowRadius, 28);
        glow.setOrigin(glowRadius, glowRadius);
        glow.setPosition(position);
        glow.setFillColor(sf::Color(255, 45, 0, 75));
        window.draw(glow, sf::BlendAdd);

        sf::CircleShape lavaBall(radius, 18);
        lavaBall.setOrigin(radius, radius);
        lavaBall.setPosition(position);
        lavaBall.setRotation(rotation);
        lavaBall.setFillColor(sf::Color(185, 48, 18));
        lavaBall.setOutlineColor(sf::Color(38, 25, 22));
        lavaBall.setOutlineThickness(std::max(1.0f, radius * 0.10f));
        window.draw(lavaBall);

        sf::RectangleShape crack({ radius * 1.28f, std::max(1.5f, radius * 0.14f) });
        crack.setOrigin(crack.getSize().x * 0.5f, crack.getSize().y * 0.5f);
        crack.setPosition(position);
        crack.setRotation(heading + rotation);
        crack.setFillColor(sf::Color(255, 180, 35, 245));
        window.draw(crack, sf::BlendAdd);
        return;
    }

    sf::CircleShape ball(radius, 40);
    ball.setOrigin(radius, radius);
    ball.setPosition(position);
    ball.setFillColor(color);
    ball.setOutlineColor(sf::Color(
        (sf::Uint8)(color.r * 0.42f),
        (sf::Uint8)(color.g * 0.42f),
        (sf::Uint8)(color.b * 0.42f)));
    ball.setOutlineThickness(std::max(1.0f, radius * 0.10f));
    window.draw(ball);

    if (movementAmount > 0.0f)
    {
        sf::RectangleShape rollingMark({ radius * 0.82f, std::max(1.5f, radius * 0.14f) });
        rollingMark.setOrigin(0.0f, rollingMark.getSize().y * 0.5f);
        rollingMark.setPosition(position);
        rollingMark.setRotation(heading);
        rollingMark.setFillColor(sf::Color(
            (sf::Uint8)(color.r * 0.55f),
            (sf::Uint8)(color.g * 0.55f),
            (sf::Uint8)(color.b * 0.55f)));
        window.draw(rollingMark);
    }

    sf::CircleShape highlight(std::max(1.5f, radius * 0.20f), 20);
    highlight.setOrigin(highlight.getRadius(), highlight.getRadius());
    highlight.setPosition(position.x - radius * 0.30f, position.y - radius * 0.32f);
    highlight.setFillColor(sf::Color(
        (sf::Uint8)(color.r + (255 - color.r) * 0.62f),
        (sf::Uint8)(color.g + (255 - color.g) * 0.62f),
        (sf::Uint8)(color.b + (255 - color.b) * 0.62f),
        210));
    window.draw(highlight);
}

void Processor_Balls::renderBallTrails(sf::RenderWindow & window)
{
    if (m_trails.empty())
    {
        return;
    }

    SandBoxProjector & projector = activeProjector();
    const cv::Mat projection = projector.getProjectionMatrix();
    const float scale = projector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    std::vector<cv::Point2f> points;
    points.reserve(m_trails.size());
    for (const BallTrail & trail : m_trails)
    {
        points.push_back(trail.position);
    }
    cv::perspectiveTransform(points, points, projection);

    const sf::Vector2f origin = projector.getTransformedPosition();
    for (size_t i = 0; i < m_trails.size(); i++)
    {
        if (!std::isfinite(points[i].x) || !std::isfinite(points[i].y))
        {
            continue;
        }

        const BallTrail & trail = m_trails[i];
        const float life = trail.lifetime > 0.0f
            ? std::clamp(trail.remaining / trail.lifetime, 0.0f, 1.0f)
            : 0.0f;
        const float radius = m_ballSize * 0.20f * (0.55f + life * 0.45f);
        sf::CircleShape mark(radius, 16);
        mark.setOrigin(radius, radius);
        mark.setPosition(origin.x + points[i].x * scale, origin.y + points[i].y * scale);
        mark.setFillColor(sf::Color(
            trail.color.r,
            trail.color.g,
            trail.color.b,
            (sf::Uint8)((trail.lava ? 110.0f : 75.0f) * life)));
        window.draw(mark, trail.lava ? sf::BlendAdd : sf::BlendAlpha);
    }
}

void Processor_Balls::renderBalls(sf::RenderWindow & window)
{
    renderBallTrails(window);
    if (m_balls.empty())
    {
        return;
    }

    SandBoxProjector & projector = activeProjector();
    const cv::Mat projection = projector.getProjectionMatrix();
    const float scale = projector.getTransformedScale();
    if (projection.empty() || !std::isfinite(scale) || scale <= 0.0f)
    {
        return;
    }

    std::vector<cv::Point2f> projectedPoints;
    projectedPoints.reserve(m_balls.size() * 2);
    for (const Ball & ball : m_balls)
    {
        cv::Point2f movementDirection = ball.velocity;
        const float velocityLength = std::sqrt(
            movementDirection.x * movementDirection.x
            + movementDirection.y * movementDirection.y);
        if (velocityLength > 0.001f)
        {
            movementDirection.x /= velocityLength;
            movementDirection.y /= velocityLength;
        }
        else
        {
            movementDirection = { 1.0f, 0.0f };
        }

        projectedPoints.push_back(ball.position);
        projectedPoints.push_back({
            ball.position.x + movementDirection.x * 4.0f,
            ball.position.y + movementDirection.y * 4.0f });
    }
    cv::perspectiveTransform(projectedPoints, projectedPoints, projection);

    const sf::Vector2f origin = projector.getTransformedPosition();
    for (size_t index = 0; index < m_balls.size(); index++)
    {
        const cv::Point2f & point = projectedPoints[index * 2];
        const cv::Point2f & ahead = projectedPoints[index * 2 + 1];
        if (!std::isfinite(point.x) || !std::isfinite(point.y)
            || !std::isfinite(ahead.x) || !std::isfinite(ahead.y))
        {
            continue;
        }

        float terrainHeight = 0.5f;
        sampleTerrainHeight(m_topography, m_balls[index].position, terrainHeight);
        const float visualScale = 0.88f + std::clamp(terrainHeight, 0.0f, 1.0f) * 0.24f;
        drawBall(
            window,
            { origin.x + point.x * scale, origin.y + point.y * scale },
            { ahead.x - point.x, ahead.y - point.y },
            m_balls[index].color,
            m_balls[index].rotation,
            visualScale,
            std::sqrt(
                m_balls[index].velocity.x * m_balls[index].velocity.x
                + m_balls[index].velocity.y * m_balls[index].velocity.y) > 0.05f
                ? 1.0f
                : 0.0f);
    }
}

void Processor_Balls::render(sf::RenderWindow & window)
{
    PROFILE_FUNCTION();

    if (m_hasFrame)
    {
        m_sprite.setPosition(m_projector.getTransformedPosition());
        const float scale = m_projector.getTransformedScale();
        m_sprite.setScale(scale, scale);

        if (m_shaderLoaded)
        {
            const sf::Vector2u textureSize = m_texture.getSize();
            m_shader.setUniform("texelSize", sf::Glsl::Vec2(1.0f / textureSize.x, 1.0f / textureSize.y));
            window.draw(m_sprite, &m_shader);
        }
        else
        {
            window.draw(m_sprite);
        }

        renderBalls(window);
    }

}

bool Processor_Balls::mapMouseToTerrain(
    const sf::Vector2f & mouse,
    cv::Point2f & terrainPosition)
{
    if (m_topography.empty())
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
    return isValidTerrainPosition(m_topography, terrainPosition);
}

void Processor_Balls::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    const bool draggingProjection = activeProjector().processEvent(event, mouse);
    if (event.type != sf::Event::MouseButtonPressed || event.mouseButton.button != sf::Mouse::Left
        || draggingProjection || ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    cv::Point2f terrainPosition;
    if (mapMouseToTerrain(mouse, terrainPosition))
    {
        addBall(terrainPosition);
    }
}

void Processor_Balls::save(Save & save) const
{
    m_projector.save(save);
}

void Processor_Balls::load(const Save & save)
{
    m_projector.load(save);
}

void Processor_Balls::processTopography(const IntermediateData & data)
{
    PROFILE_FUNCTION();

    if (data.topography.empty() || data.topography.type() != CV_32F)
    {
        m_hasFrame = false;
        return;
    }

    if (m_topographySize.width > 0 && m_topographySize.height > 0
        && m_topographySize != data.topography.size())
    {
        const float xScale = (float)data.topography.cols / m_topographySize.width;
        const float yScale = (float)data.topography.rows / m_topographySize.height;
        for (Ball & ball : m_balls)
        {
            ball.position.x *= xScale;
            ball.position.y *= yScale;
            ball.velocity.x *= xScale;
            ball.velocity.y *= yScale;
        }
        for (BallTrail & trail : m_trails)
        {
            trail.position.x *= xScale;
            trail.position.y *= yScale;
        }
    }

    m_topography = data.topography;
    m_topographySize = data.topography.size();
    updateBalls(m_topography, data.deltaTime);

    m_projector.project(m_topography, m_projectedTopography);
    if (m_projectedTopography.empty())
    {
        m_hasFrame = false;
        return;
    }

    m_image = Tools::matToSfImage(m_projectedTopography);
    m_texture.loadFromImage(m_image);
    m_texture.setSmooth(true);
    m_sprite.setTexture(m_texture, true);
    m_hasFrame = true;
}

void Processor_Balls::initOverlay()
{
    reloadShader();
    m_balls.clear();
    m_trails.clear();
    m_defaultBallCreated = false;
    m_randomResetPending = false;
}

void Processor_Balls::imguiOverlay()
{
    PROFILE_FUNCTION();

    ImGui::Text("Balls: %d", (int)m_balls.size());
    ImGui::SliderFloat("Gravity", &m_gravity, 0.0f, 5000.0f, "%.0f");
    ImGui::SliderFloat("Ball Speed Multiplier", &m_ballSpeedMultiplier, 0.1f, 4.0f, "%.1fx");
    ImGui::SliderFloat("Rolling Resistance", &m_rollingResistance, 0.0f, 4.0f);
    ImGui::SliderFloat("Ball Size", &m_ballSize, 6.0f, 40.0f);
    ImGui::SliderFloat("Ball Bounciness", &m_ballRestitution, 0.0f, 1.0f);
    if (ImGui::Checkbox("Lava Appearance", &m_lavaAppearance)
        && m_lavaAppearance && m_trailLength <= 0.0f)
    {
        m_trailLength = 3.0f;
    }
    ImGui::SliderFloat("Trail Length", &m_trailLength, 0.0f, 10.0f, "%.1f sec");
    ImGui::TextUnformatted("Left mouse: add ball");

    if (ImGui::Button("Reset Balls"))
    {
        resetBalls();
    }
}

void Processor_Balls::processTopographyOverlay(
    const IntermediateData & data,
    TopographyProcessor & processor)
{
    if (data.topography.empty() || data.topography.type() != CV_32F)
    {
        return;
    }

    m_overlayProcessor = &processor;
    if (m_topographySize.width > 0 && m_topographySize.height > 0
        && m_topographySize != data.topography.size())
    {
        const float xScale = (float)data.topography.cols / m_topographySize.width;
        const float yScale = (float)data.topography.rows / m_topographySize.height;
        for (Ball & ball : m_balls)
        {
            ball.position.x *= xScale;
            ball.position.y *= yScale;
            ball.velocity.x *= xScale;
            ball.velocity.y *= yScale;
        }
        for (BallTrail & trail : m_trails)
        {
            trail.position.x *= xScale;
            trail.position.y *= yScale;
        }
    }

    m_topography = data.topography;
    m_topographySize = data.topography.size();
    updateBalls(m_topography, data.deltaTime);
}

void Processor_Balls::renderOverlay(
    sf::RenderWindow & window,
    TopographyProcessor & processor)
{
    m_overlayProcessor = &processor;
    renderBalls(window);
}

void Processor_Balls::processOverlayEvent(
    const sf::Event & event,
    const sf::Vector2f & mouse,
    TopographyProcessor & processor)
{
    m_overlayProcessor = &processor;
    processEvent(event, mouse);
}

void Processor_Balls::saveOverlay(Save &) const
{
}

void Processor_Balls::loadOverlay(const Save &)
{
}
