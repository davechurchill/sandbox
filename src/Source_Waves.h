#pragma once

#include "TopographySource.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

class Source_Waves : public TopographySource
{
    static constexpr int CanvasSize = 512;

    cv::Mat             m_topography;

    sf::Image           m_image;
    sf::Texture         m_texture;
    sf::Sprite          m_sprite{ m_texture };
    sf::Clock           m_clock;

    int                 m_animationMode = 1;
    float               m_frequency = 0.35f;
    float               m_amplitude = 0.47f;
    float               m_separation = 96.0f;
    float               m_waveSize = 32.0f;
    float               m_phase = 0.0f;
    bool                m_textureDirty = true;

    void updateTopography();
    void updateTexture();

public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Settings & save) const;
    void load(const Settings & save);

    cv::Mat getTopography();
};
