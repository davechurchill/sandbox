#pragma once

#include "Settings.hpp"
#include "MarkerData.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <cstdint>

class TopographySource
{
    std::uint64_t m_revision = 0;

protected:
    void markTerrainChanged() { ++m_revision; }

public:
    virtual ~TopographySource() = default;

    virtual void init() = 0;
    virtual void activate() {}
    virtual void deactivate() {}
    virtual void reset() {}
    virtual bool usesProjectedInput() const { return false; }
    virtual void imgui() = 0;
    virtual void render(sf::RenderWindow & window) = 0;
    virtual void processEvent(const sf::Event & event, const sf::Vector2f & mouse) = 0;
    virtual void save(Settings & save) const = 0;
    virtual void load(const Settings & save) = 0;

    virtual cv::Mat getTopography() = 0;
    virtual std::vector<MarkerData> getMarkers() { return {}; }
    std::uint64_t revision() const { return m_revision; }
};
