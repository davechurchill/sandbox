#include "SandboxSession.hpp"

#include "Overlay_AStar.h"
#include "Overlay_Animals.h"
#include "Overlay_BFS.h"
#include "Overlay_Balls.h"
#include "Overlay_Cloth.h"
#include "Overlay_ColorAdjustment.h"
#include "Overlay_ContourLines.h"
#include "Overlay_SmokeFire.h"
#include "Overlay_Vectors.h"
#include "Overlay_Weather.h"
#include "Overlay_WaterFlow.h"
#include "Processor_Colorizer.h"
#include "Processor_FishPond.h"
#include "Processor_ForestFire.h"
#include "Processor_Heat.h"
#include "Processor_Blockworld.h"
#include "Processor_Nature.h"
#include "Processor_TerrainLighting.h"
#include "Source_Camera.h"
#include "Source_PaintBrush.h"
#include "Source_Perlin.h"
#include "Source_Snapshot.h"
#include "Source_Waves.h"

namespace
{
    std::shared_ptr<TopographySource> createSource(const std::string & name)
    {
        if (name == "Camera") return std::make_shared<Source_Camera>();
        if (name == "PaintBrush") return std::make_shared<Source_PaintBrush>();
        if (name == "Perlin") return std::make_shared<Source_Perlin>();
        if (name == "Snapshot") return std::make_shared<Source_Snapshot>();
        if (name == "Waves") return std::make_shared<Source_Waves>();
        return nullptr;
    }

    std::shared_ptr<TopographyProcessor> createProcessor(
        const std::string & name,
        SandBoxProjector & projector)
    {
        std::shared_ptr<TopographyProcessor> processor;
        if (name == "Blockworld") processor = std::make_shared<Processor_Blockworld>();
        else if (name == "Colorizer") processor = std::make_shared<Processor_Colorizer>();
        else if (name == "Fish Pond") processor = std::make_shared<Processor_FishPond>();
        else if (name == "Forest Fire") processor = std::make_shared<Processor_ForestFire>();
        else if (name == "Heat") processor = std::make_shared<Processor_Heat>();
        else if (name == "Nature") processor = std::make_shared<Processor_Nature>();
        else if (name == "TerrainLighting") processor = std::make_shared<Processor_TerrainLighting>();

        if (processor)
        {
            processor->setProjector(projector);
        }
        return processor;
    }

    std::shared_ptr<TopographyOverlay> createOverlay(const std::string & name)
    {
        if (name == "Adjust Terrain Color") return std::make_shared<Overlay_ColorAdjustment>();
        if (name == "Animals") return std::make_shared<Overlay_Animals>();
        if (name == "BFS") return std::make_shared<Overlay_BFS>();
        if (name == "Balls") return std::make_shared<Overlay_Balls>();
        if (name == "Cloth Sheet") return std::make_shared<Overlay_Cloth>();
        if (name == "Contour Lines") return std::make_shared<Overlay_ContourLines>();
        if (name == "Pathfinding (A*)") return std::make_shared<Overlay_AStar>();
        if (name == "Smoke and Fire") return std::make_shared<Overlay_SmokeFire>();
        if (name == "Vectors") return std::make_shared<Overlay_Vectors>();
        if (name == "WaterFlow") return std::make_shared<Overlay_WaterFlow>();
        if (name == "Weather") return std::make_shared<Overlay_Weather>();
        return nullptr;
    }
}

SandboxSession::SandboxSession()
{
    setOverlay(m_overlayID, false);
}

void SandboxSession::processFrame(float deltaTime)
{
    m_topography = m_source->getTopography();
    if (m_processor && m_topography.rows > 0 && m_topography.cols > 0)
    {
        IntermediateData data;
        data.deltaTime = deltaTime;
        data.topography = m_topography;
        data.markers = m_source->getMarkers();
        m_processor->processTopography(data);
        for (auto & [_, state] : m_overlayStates)
        {
            if (state.enabled && state.overlay)
            {
                state.overlay->processTopographyOverlay(data, *m_processor);
            }
        }
    }
}

std::vector<std::string> SandboxSession::sourceNames() const
{
    return { "Camera", "PaintBrush", "Perlin", "Snapshot", "Waves" };
}

std::vector<std::string> SandboxSession::processorNames() const
{
    return {
        "None",
        "Blockworld",
        "Colorizer",
        "Fish Pond",
        "Forest Fire",
        "Heat",
        "Nature",
        "TerrainLighting"
    };
}

std::vector<std::string> SandboxSession::overlayNames() const
{
    return {
        "Adjust Terrain Color",
        "Animals",
        "BFS",
        "Balls",
        "Cloth Sheet",
        "Contour Lines",
        "Pathfinding (A*)",
        "Smoke and Fire",
        "Vectors",
        "WaterFlow",
        "Weather"
    };
}

SandboxSession::OverlayState * SandboxSession::ensureOverlay(const std::string & name)
{
    if (name == "None")
    {
        return nullptr;
    }

    const auto found = m_overlayStates.find(name);
    if (found != m_overlayStates.end())
    {
        return found->second.overlay ? &found->second : nullptr;
    }

    std::shared_ptr<TopographyOverlay> overlay = createOverlay(name);
    if (!overlay)
    {
        return nullptr;
    }

    overlay->initOverlay();
    overlay->loadOverlay(m_settings);
    const auto inserted = m_overlayStates.emplace(
        name,
        OverlayState{ std::move(overlay), false });
    return &inserted.first->second;
}

TopographyOverlay * SandboxSession::overlay() const
{
    const auto found = m_overlayStates.find(m_overlayID);
    return found != m_overlayStates.end() ? found->second.overlay.get() : nullptr;
}

TopographyOverlay * SandboxSession::inputOverlay() const
{
    const auto found = m_overlayStates.find(m_overlayID);
    return found != m_overlayStates.end() && found->second.enabled
        ? found->second.overlay.get()
        : nullptr;
}

bool SandboxSession::overlayEnabled(const std::string & name) const
{
    const auto found = m_overlayStates.find(name);
    return found != m_overlayStates.end() && found->second.enabled;
}

void SandboxSession::setSource(const std::string & source, bool saveCurrent)
{
    const bool sourceChanged = !m_source || source != m_sourceID;
    if (saveCurrent && m_source) { m_source->save(m_settings); }
    m_sourceID = source;
    m_source = createSource(source);
    if (!m_source)
    {
        m_source = createSource("Camera");
    }
    if (m_source)
    {
        m_source->init();
        m_source->load(m_settings);
    }
    if (sourceChanged && m_processor)
    {
        m_processor->onSourceChanged();
    }
}

void SandboxSession::setProcessor(const std::string & processor, bool saveCurrent)
{
    if (saveCurrent && m_processor) { m_processor->save(m_settings); }
    m_processorID = processor;
    m_processor = createProcessor(processor, m_projector);
    if (!m_processor && processor != "None")
    {
        m_processor = createProcessor("Colorizer", m_projector);
    }
    if (m_processor)
    {
        m_processor->init();
        m_processor->load(m_settings);
    }
}

void SandboxSession::setOverlay(const std::string & overlay, bool saveCurrent)
{
    if (saveCurrent)
    {
        if (TopographyOverlay * current = this->overlay())
        {
            current->saveOverlay(m_settings);
        }
    }

    std::string selection = overlay;
    OverlayState * state = ensureOverlay(selection);
    if (!state)
    {
        selection = overlayNames().front();
        state = ensureOverlay(selection);
    }

    if (state)
    {
        m_overlayID = selection;
    }
    else
    {
        m_overlayID.clear();
    }
}

void SandboxSession::setOverlayEnabled(const std::string & overlay, bool enabled)
{
    if (OverlayState * state = ensureOverlay(overlay))
    {
        state->enabled = enabled;
    }
}

void SandboxSession::renderOverlays(sf::RenderWindow & window)
{
    if (!m_processor)
    {
        return;
    }

    for (auto & [_, state] : m_overlayStates)
    {
        if (state.enabled && state.overlay)
        {
            state.overlay->renderOverlay(window, *m_processor);
        }
    }
}

void SandboxSession::saveSettings(const std::string & filename, bool doubleSizeUI, const std::string & displayMonitorID)
{
    m_projector.save(m_settings);
    if (m_source) { m_source->save(m_settings); }
    if (m_processor) { m_processor->save(m_settings); }
    for (const auto & [_, state] : m_overlayStates)
    {
        if (state.overlay) { state.overlay->saveOverlay(m_settings); }
    }

    Settings::json & settings = m_settings.section("Scene_Main");
    settings["m_sourceID"] = m_sourceID;
    settings["m_processorID"] = m_processorID;
    settings["m_doubleSizeUI"] = doubleSizeUI;

    Settings::json & overlaySettings = m_settings.section("Overlays");
    overlaySettings["m_enabledOverlayIDs"] = Settings::json::array();
    for (const auto & [name, state] : m_overlayStates)
    {
        if (state.enabled)
        {
            overlaySettings["m_enabledOverlayIDs"].push_back(name);
        }
    }
    overlaySettings["m_selectedOverlayID"] = m_overlayID;

    m_settings.section("Projection")["m_displayMonitorID"] = displayMonitorID;

    m_settings.saveToFile(filename);
}

bool SandboxSession::loadSettings(const std::string & filename, bool & doubleSizeUI, std::string & displayMonitorID)
{
    if (!m_settings.loadFromFile(filename) && m_source)
    {
        return false;
    }

    m_projector.load(m_settings);

    const Settings::json & settings = m_settings.section("Scene_Main");
    std::string source = m_sourceID;
    std::string processor = m_processorID;
    Settings::read(settings, "m_sourceID", source);
    Settings::read(settings, "m_processorID", processor);
    Settings::read(settings, "m_doubleSizeUI", doubleSizeUI);
    Settings::read(m_settings.section("Projection"), "m_displayMonitorID", displayMonitorID);

    std::vector<std::string> enabledOverlays;
    std::string selectedOverlay = m_overlayID;
    const Settings::json & overlaySettings = m_settings.section("Overlays");
    const auto enabledIDs = overlaySettings.find("m_enabledOverlayIDs");
    if (enabledIDs != overlaySettings.end() && enabledIDs->is_array())
    {
        for (const Settings::json & id : *enabledIDs)
        {
            if (id.is_string())
            {
                enabledOverlays.push_back(id.get<std::string>());
            }
        }
    }
    Settings::read(overlaySettings, "m_selectedOverlayID", selectedOverlay);

    setSource(source, false);
    setProcessor(processor, false);
    m_overlayStates.clear();
    for (const std::string & id : enabledOverlays)
    {
        if (OverlayState * state = ensureOverlay(id))
        {
            state->enabled = true;
        }
    }
    setOverlay(selectedOverlay, false);
    return true;
}
