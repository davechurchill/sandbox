#pragma once

#include "SandboxProjector.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class ProjectedSurface
{
    cv::Mat m_projectedImage;
    sf::Image m_image;
    sf::Texture m_texture;
    sf::Sprite m_sprite{ m_texture };
    bool m_ready = false;

public:
    [[nodiscard]] bool update(
        const cv::Mat & source,
        SandBoxProjector & projector,
        bool smooth,
        const char * textureErrorMessage);

    void draw(
        sf::RenderWindow & window,
        const SandBoxProjector & projector,
        sf::Shader * shader = nullptr);

    bool ready() const { return m_ready; }
    const sf::Texture & texture() const { return m_texture; }
};
