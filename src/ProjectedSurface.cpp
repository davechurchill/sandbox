#include "ProjectedSurface.h"

#include "Profiler.hpp"
#include "Tools.h"

#include <iostream>

bool ProjectedSurface::update(
    const cv::Mat & source,
    SandBoxProjector & projector,
    bool smooth,
    const char * textureErrorMessage)
{
    PROFILE_FUNCTION();

    {
        PROFILE_SCOPE("Calibration TransformProjection");
        projector.project(source, m_projectedImage);
    }
    if (m_projectedImage.empty())
    {
        m_ready = false;
        return false;
    }

    m_image = Tools::matToSfImage(m_projectedImage);
    if (!m_texture.loadFromImage(m_image))
    {
        if (textureErrorMessage)
        {
            std::cerr << textureErrorMessage;
        }
        return m_ready;
    }

    m_texture.setSmooth(smooth);
    m_sprite.setTexture(m_texture, true);
    m_ready = true;
    return true;
}

void ProjectedSurface::draw(
    sf::RenderWindow & window,
    const SandBoxProjector & projector,
    sf::Shader * shader)
{
    if (!m_ready)
    {
        return;
    }

    m_sprite.setPosition(projector.getTransformedPosition());
    const float scale = projector.getTransformedScale();
    m_sprite.setScale({ scale, scale });

    if (shader)
    {
        window.draw(m_sprite, shader);
    }
    else
    {
        window.draw(m_sprite);
    }
}
