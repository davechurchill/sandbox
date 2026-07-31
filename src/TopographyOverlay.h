#pragma once

#include "TopographyProcessor.h"

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
    virtual void saveOverlay(Save & save) const = 0;
    virtual void loadOverlay(const Save & save) = 0;
};
