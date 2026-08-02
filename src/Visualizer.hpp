#pragma once

#include "SandboxProjector.h"
#include "Settings.hpp"
#include "TerrainFrame.hpp"

#include <SFML/Graphics.hpp>
#include <string>
#include <string_view>

class Visualizer
{
    SandboxProjector & m_projector;
    const std::string m_name;

protected:
    Visualizer(std::string_view name, SandboxProjector & projector)
        : m_projector(projector)
        , m_name(name)
    {
    }

    SandboxProjector & projector() const { return m_projector; }

public:
    virtual ~Visualizer() = default;

    std::string_view name() const { return m_name; }

    virtual void init() = 0;
    virtual void imgui() = 0;
    virtual void process(const TerrainFrame &) {}
    virtual void render(sf::RenderWindow & window) = 0;
    virtual void processEvent(const sf::Event &, const sf::Vector2f &) {}
    virtual void save(Settings & settings) const = 0;
    virtual void load(const Settings & settings) = 0;

    virtual void onSourceChanged() {}
    virtual bool usesCanvasInput() const { return false; }
};
