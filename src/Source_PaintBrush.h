#pragma once

#include "TopographySource.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Source_PaintBrush : public TopographySource
{
    static constexpr int CanvasSize = 512;
    static constexpr float DefaultHeight = 0.5f;

    cv::Mat             m_topography;

    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite{ m_texture };

    float               m_brushSize = 30.0f;
    float               m_brushBlur = 12.0f;
    float               m_paintAmount = 0.0025f;
    bool                m_textureDirty = true;
    bool                m_hasLastPaintPosition = false;
    sf::Vector2f        m_lastPaintPosition;

    void resetCanvas();
    void applyBrush(const sf::Vector2f & position, float direction);
    void paintLine(const sf::Vector2f & from, const sf::Vector2f & to, float direction);
    void updateTexture();

public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);

    cv::Mat getTopography();
};
