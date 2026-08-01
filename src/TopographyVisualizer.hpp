#pragma once

#include "SandboxProjector.h"
#include "Settings.hpp"
#include "TerrainContext.hpp"
#include "TerrainFrame.hpp"

#include <SFML/Graphics.hpp>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <string>
#include <string_view>

class TopographyVisualizer
{
    TerrainContext * m_context = nullptr;
    const std::string m_name;

protected:
    explicit TopographyVisualizer(std::string_view name)
        : m_name(name)
    {
    }

    TerrainContext & context() const
    {
        assert(m_context);
        return *m_context;
    }

    SandBoxProjector & projector() const
    {
        return context().projector();
    }

public:
    virtual ~TopographyVisualizer() = default;

    std::string_view name() const { return m_name; }

    void setContext(TerrainContext & context) { m_context = &context; }

    virtual void init() = 0;
    virtual void activate() {}
    virtual void deactivate() {}
    virtual void reset() {}
    virtual void imgui() = 0;
    virtual void process(const TerrainFrame & data) = 0;
    virtual void render(sf::RenderWindow & window) = 0;
    virtual void processEvent(
        const sf::Event & event,
        const sf::Vector2f & mouse) = 0;
    virtual void save(Settings & settings) const = 0;
    virtual void load(const Settings & settings) = 0;

    virtual void onSourceChanged() {}
    virtual bool usesCanvasInput() const { return false; }
    virtual bool definesTerrainWalkability() const { return false; }

    virtual bool isTerrainWalkable(
        const cv::Mat & terrain,
        const cv::Point2f & position) const
    {
        return TerrainContext::defaultTerrainWalkability(terrain, position);
    }
};
