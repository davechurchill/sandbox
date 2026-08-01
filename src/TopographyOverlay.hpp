#pragma once

#include "TopographyProcessor.hpp"

class TopographyOverlay
{
public:
    virtual ~TopographyOverlay() = default;

    virtual bool usesCanvasInput() const { return true; }
    virtual void initOverlay() = 0;
    virtual void imguiOverlay() = 0;
    virtual void processTopographyOverlay(
        const IntermediateData & data,
        TopographyProcessor & processor) = 0;
    virtual void renderOverlay(
        sf::RenderWindow & window,
        TopographyProcessor & processor) = 0;
    virtual void processOverlayEvent(
        const sf::Event & event,
        const sf::Vector2f & mouse,
        TopographyProcessor & processor) = 0;
    virtual void saveOverlay(Settings & save) const = 0;
    virtual void loadOverlay(const Settings & save) = 0;
};
