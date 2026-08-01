#include "Source_PaintBrush.h"
#include "Tools.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace
{
    constexpr float MinimumDrawableHeight = 6.0f / 255.0f;
    constexpr float MaximumDrawableHeight = 252.0f / 255.0f;
}

void Source_PaintBrush::init()
{
    resetCanvas();
}

void Source_PaintBrush::resetCanvas()
{
    m_topography = cv::Mat(cv::Size(CanvasSize, CanvasSize), CV_32F, cv::Scalar(DefaultHeight));
    m_textureDirty = true;
    m_hasLastPaintPosition = false;
}

void Source_PaintBrush::applyBrush(const sf::Vector2f & position, float direction)
{
    const float radius = std::max(m_brushSize, 1.0f);
    const float sigma = std::max(m_brushBlur, 0.1f);
    const float radiusSquared = radius * radius;
    const float gaussianDenominator = 2.0f * sigma * sigma;

    const int minX = std::max(0, (int)std::floor(position.x - radius));
    const int maxX = std::min(m_topography.cols - 1, (int)std::ceil(position.x + radius));
    const int minY = std::max(0, (int)std::floor(position.y - radius));
    const int maxY = std::min(m_topography.rows - 1, (int)std::ceil(position.y + radius));

    if (minX > maxX || minY > maxY)
    {
        return;
    }

    for (int y = minY; y <= maxY; y++)
    {
        float * row = m_topography.ptr<float>(y);
        for (int x = minX; x <= maxX; x++)
        {
            const float dx = x - position.x;
            const float dy = y - position.y;
            const float distanceSquared = dx * dx + dy * dy;
            if (distanceSquared <= radiusSquared)
            {
                const float weight = std::exp(-distanceSquared / gaussianDenominator);
                row[x] = std::clamp(
                    row[x] + direction * m_paintAmount * weight,
                    MinimumDrawableHeight,
                    MaximumDrawableHeight);
            }
        }
    }

    m_textureDirty = true;
}

void Source_PaintBrush::paintLine(const sf::Vector2f & from, const sf::Vector2f & to, float direction)
{
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const float distance = std::sqrt(dx * dx + dy * dy);
    const float spacing = std::max(1.0f, m_brushSize * 0.25f);
    const int steps = std::max(1, (int)std::ceil(distance / spacing));

    for (int step = 1; step <= steps; step++)
    {
        const float amount = (float)step / steps;
        applyBrush({ from.x + dx * amount, from.y + dy * amount }, direction);
    }
}

void Source_PaintBrush::updateTexture()
{
    if (!m_textureDirty)
    {
        return;
    }

    m_image = Tools::matToSfImage(m_topography);
    if (!m_texture.loadFromImage(m_image))
    {
        std::cerr << "Failed to load the paintbrush terrain texture.\n";
        return;
    }
    m_sprite.setTexture(m_texture, true);
    m_textureDirty = false;
}

void Source_PaintBrush::imgui()
{
    ImGui::SliderFloat("Brush Size", &m_brushSize, 1.0f, 128.0f);
    ImGui::SliderFloat("Brush Blur", &m_brushBlur, 0.5f, 64.0f);
    ImGui::SliderFloat("Raise / Lower Amount", &m_paintAmount, 0.0001f, 0.025f, "%.4f");

    if (ImGui::Button("Reset Height"))
    {
        resetCanvas();
    }

    ImGui::TextUnformatted("Left mouse: raise terrain");
    ImGui::TextUnformatted("Middle mouse: lower terrain");
}

void Source_PaintBrush::render(sf::RenderWindow & window)
{
    updateTexture();
    window.draw(m_sprite);
}

void Source_PaintBrush::processEvent(const sf::Event & event, const sf::Vector2f & mouse)
{
    if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>())
    {
        if (mouseReleased->button == sf::Mouse::Button::Left || mouseReleased->button == sf::Mouse::Button::Middle)
        {
            m_hasLastPaintPosition = false;
        }
        return;
    }

    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    float direction = 0.0f;
    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
    {
        direction = 1.0f;
    }
    else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle))
    {
        direction = -1.0f;
    }

    if (event.is<sf::Event::MouseButtonPressed>() && direction != 0.0f)
    {
        applyBrush(mouse, direction);
        m_lastPaintPosition = mouse;
        m_hasLastPaintPosition = true;
    }
    else if (event.is<sf::Event::MouseMoved>())
    {
        if (direction == 0.0f)
        {
            m_hasLastPaintPosition = false;
            return;
        }

        if (m_hasLastPaintPosition)
        {
            paintLine(m_lastPaintPosition, mouse, direction);
        }
        else
        {
            applyBrush(mouse, direction);
        }

        m_lastPaintPosition = mouse;
        m_hasLastPaintPosition = true;
    }
}

void Source_PaintBrush::save(Save & save) const
{
    Save::Json & settings = save.section("Source_PaintBrush");
    settings["m_brushSize"] = m_brushSize;
    settings["m_brushBlur"] = m_brushBlur;
    settings["m_paintAmount"] = m_paintAmount;
}

void Source_PaintBrush::load(const Save & save)
{
    const Save::Json & settings = save.section("Source_PaintBrush");
    Save::read(settings, "m_brushSize", m_brushSize);
    Save::read(settings, "m_brushBlur", m_brushBlur);
    Save::read(settings, "m_paintAmount", m_paintAmount);
}

cv::Mat Source_PaintBrush::getTopography()
{
    return m_topography;
}
