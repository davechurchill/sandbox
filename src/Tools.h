#pragma once

#include <SFML/Graphics.hpp>
#include <opencv2/opencv.hpp>   // Include OpenCV API

class SandBoxProjector;

namespace Tools
{
    // given an (mx, my) mouse position, return the index of the first circle the contains the position
    // returns -1 if the mouse position is not inside any circle
    int getClickedCircleIndex(float mx, float my, std::vector<sf::CircleShape> & circles);

    sf::Image matToSfImage(const cv::Mat & mat);

    [[nodiscard]] bool updateProjectedTexture(
        const cv::Mat & source,
        SandBoxProjector & projector,
        cv::Mat & projectedImage,
        sf::Image & image,
        sf::Texture & texture,
        sf::Sprite & sprite,
        bool smooth,
        const char * textureErrorMessage);
}
