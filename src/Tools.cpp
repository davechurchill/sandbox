#pragma once

#include "Tools.h"
#include "Profiler.hpp"
#include "SandboxProjector.h"

#include <iostream>

namespace Tools
{
    // given an (mx, my) mouse position, return the index of the first circle the contains the position
    // returns -1 if the mouse position is not inside any circle
    int getClickedCircleIndex(float mx, float my, std::vector<sf::CircleShape> & circles)
    {
        PROFILE_FUNCTION();
        for (int i = 0; i < circles.size(); i++)
        {
            float dx = mx - circles[i].getPosition().x;
            float dy = my - circles[i].getPosition().y;
            float d2 = dx * dx + dy * dy;
            float rad2 = circles[i].getRadius() * circles[i].getRadius();
            if (d2 <= rad2) { return i; }
        }

        return -1;
    }

    sf::Image matToSfImage(const cv::Mat & mat)
    {
        PROFILE_FUNCTION();

        // Ensure the input image is in the correct format (CV_32F)
        cv::Mat normalized;
        mat.convertTo(normalized, CV_8U, 255.0); // Scale float [0, 1] to [0, 255]

        // Convert to RGB (SFML requires RGB format)
        cv::Mat rgb;
        cv::cvtColor(normalized, rgb, cv::COLOR_GRAY2RGBA);

        // Create SFML image
        sf::Image image;
        image.resize({ (unsigned int)rgb.cols, (unsigned int)rgb.rows }, rgb.ptr());

        return image;
    }

    bool updateProjectedTexture(
        const cv::Mat & source,
        SandBoxProjector & projector,
        cv::Mat & projectedImage,
        sf::Image & image,
        sf::Texture & texture,
        sf::Sprite & sprite,
        bool smooth,
        const char * textureErrorMessage)
    {
        PROFILE_FUNCTION();

        {
            PROFILE_SCOPE("Calibration TransformProjection");
            projector.project(source, projectedImage);
        }
        if (projectedImage.empty())
        {
            return false;
        }

        image = matToSfImage(projectedImage);
        if (!texture.loadFromImage(image))
        {
            if (textureErrorMessage)
            {
                std::cerr << textureErrorMessage;
            }
            const sf::Vector2u textureSize = texture.getSize();
            return textureSize.x > 0 && textureSize.y > 0;
        }

        texture.setSmooth(smooth);
        sprite.setTexture(texture, true);
        return true;
    }
}
