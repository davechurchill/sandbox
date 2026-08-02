#pragma once

#include "MarkerDetector.hpp"
#include "Settings.hpp"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <cstdint>

class Source
{
    std::uint64_t m_revision = 0;

protected:
    void markTerrainChanged() { ++m_revision; }

public:
    virtual ~Source() = default;

    virtual void init() {}
    virtual bool usesProjectedInput() const { return false; }
    virtual void imgui() = 0;
    virtual void render(sf::RenderWindow & window) = 0;
    virtual void processEvent(const sf::Event &, const sf::Vector2f &) {}
    virtual void save(Settings & save) const = 0;
    virtual void load(const Settings & save) = 0;

    virtual cv::Mat getTopography() = 0;
    virtual std::vector<MarkerData> getMarkers() { return {}; }
    std::uint64_t revision() const { return m_revision; }
};
