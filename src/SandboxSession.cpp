#include "SandboxSession.hpp"

#include "Overlay_Animals.h"
#include "Overlay_BFS.h"
#include "Overlay_Balls.h"
#include "Overlay_Cloth.h"
#include "Overlay_CloudSimulation.h"
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

#include <algorithm>

namespace
{
    template <typename Factory>
    std::vector<std::string> registeredNames(const std::map<std::string, Factory> & factories)
    {
        std::vector<std::string> names;
        names.reserve(factories.size());
        if (factories.contains("None"))
        {
            names.push_back("None");
        }
        for (const auto & [name, _] : factories)
        {
            if (name != "None")
            {
                names.push_back(name);
            }
        }
        return names;
    }
}

SandboxSession::SandboxSession()
{
    registerComponents();
    setOverlay(m_overlayID, false);
}

void SandboxSession::registerComponents()
{
    registerSource<Source_Camera>("Camera");
    registerSource<Source_PaintBrush>("PaintBrush");
    registerSource<Source_Perlin>("Perlin");
    registerSource<Source_Snapshot>("Snapshot");
    registerSource<Source_Waves>("Waves");

    registerProcessor<Processor_Colorizer>("Colorizer");
    registerProcessor<Processor_FishPond>("Fish Pond");
    registerProcessor<Processor_ForestFire>("Forest Fire");
    registerProcessor<Processor_Heat>("Heat");
    registerProcessor<Processor_Blockworld>("Blockworld");
    registerProcessor<Processor_Nature>("Nature");
    registerProcessor<Processor_TerrainLighting>("TerrainLighting");
    m_processorMap.emplace("None", [](SandBoxProjector &) { return nullptr; });

    registerOverlay<Overlay_Animals>("Animals");
    registerOverlay<Overlay_BFS>("BFS");
    registerOverlay<Overlay_Balls>("Balls");
    registerOverlay<Overlay_Cloth>("Cloth Sheet");
    registerOverlay<Overlay_CloudSimulation>("Cloud Simulation");
    registerOverlay<Overlay_ContourLines>("Contour Lines");
    registerOverlay<Overlay_SmokeFire>("Smoke and Fire");
    registerOverlay<Overlay_Vectors>("Vectors");
    registerOverlay<Overlay_Weather>("Weather");
    registerOverlay<Overlay_WaterFlow>("WaterFlow");
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
    return registeredNames(m_sourceMap);
}

std::vector<std::string> SandboxSession::processorNames() const
{
    return registeredNames(m_processorMap);
}

std::vector<std::string> SandboxSession::overlayNames() const
{
    return registeredNames(m_overlayMap);
}

SandboxSession::OverlayState * SandboxSession::ensureOverlay(const std::string & name)
{
    if (name == "None" || !m_overlayMap.contains(name))
    {
        return nullptr;
    }

    OverlayState & state = m_overlayStates[name];
    if (!state.overlay)
    {
        state.overlay = m_overlayMap.at(name)();
        if (state.overlay)
        {
            state.overlay->initOverlay();
            state.overlay->loadOverlay(m_save);
        }
    }
    return state.overlay ? &state : nullptr;
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
    if (saveCurrent && m_source) { m_source->save(m_save); }
    m_sourceID = source;
    if (m_sourceMap.contains(source))
    {
        m_source = m_sourceMap.at(source)();
    }
    else
    {
        m_source = m_sourceMap.at("Camera")();
    }
    if (m_source)
    {
        m_source->init();
        m_source->load(m_save);
    }
    if (sourceChanged && m_processor)
    {
        m_processor->onSourceChanged();
    }
}

void SandboxSession::setProcessor(const std::string & processor, bool saveCurrent)
{
    if (saveCurrent && m_processor) { m_processor->save(m_save); }
    m_processorID = processor;
    if (m_processorMap.contains(processor))
    {
        m_processor = m_processorMap.at(processor)(m_projector);
    }
    else
    {
        m_processor = m_processorMap.at("Colorizer")(m_projector);
    }
    if (m_processor)
    {
        m_processor->init();
        m_processor->load(m_save);
    }
}

void SandboxSession::setOverlay(const std::string & overlay, bool saveCurrent)
{
    if (saveCurrent)
    {
        if (TopographyOverlay * current = this->overlay())
        {
            current->saveOverlay(m_save);
        }
    }

    std::string selection = overlay;
    if (!m_overlayMap.contains(selection))
    {
        if (m_overlayMap.empty())
        {
            m_overlayID.clear();
            return;
        }
        selection = m_overlayMap.begin()->first;
    }

    if (ensureOverlay(selection))
    {
        m_overlayID = selection;
    }
    else
    {
        m_overlayID.clear();
    }
}

void SandboxSession::setOverlayEnabled(
    const std::string & overlay,
    bool enabled)
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

void SandboxSession::saveSettings(
    const std::string & filename,
    bool doubleSizeUI,
    const std::string & displayMonitorID)
{
    m_projector.save(m_save);
    if (m_source) { m_source->save(m_save); }
    if (m_processor) { m_processor->save(m_save); }
    for (const auto & [_, state] : m_overlayStates)
    {
        if (state.overlay) { state.overlay->saveOverlay(m_save); }
    }

    Save::Json & settings = m_save.section("Scene_Main");
    settings["m_sourceID"] = m_sourceID;
    settings["m_processorID"] = m_processorID;
    settings.erase("m_overlayID");
    settings["m_doubleSizeUI"] = doubleSizeUI;

    Save::Json & overlaySettings = m_save.section("Overlays");
    overlaySettings["m_enabledOverlayIDs"] = Save::Json::array();
    for (const auto & [name, state] : m_overlayStates)
    {
        if (state.enabled)
        {
            overlaySettings["m_enabledOverlayIDs"].push_back(name);
        }
    }
    overlaySettings["m_selectedOverlayID"] = m_overlayID;

    m_save.section("Projection")["m_displayMonitorID"] = displayMonitorID;

    m_save.saveToFile(filename);
}

bool SandboxSession::loadSettings(
    const std::string & filename,
    bool & doubleSizeUI,
    std::string & displayMonitorID)
{
    if (!m_save.loadFromFile(filename) && m_source)
    {
        return false;
    }

    m_projector.load(m_save);

    const Save::Json & settings = m_save.section("Scene_Main");
    std::string source = m_sourceID;
    std::string processor = m_processorID;
    std::string legacyOverlay;
    Save::read(settings, "m_sourceID", source);
    Save::read(settings, "m_processorID", processor);
    Save::read(settings, "m_overlayID", legacyOverlay);
    Save::read(settings, "m_doubleSizeUI", doubleSizeUI);
    Save::read(
        m_save.section("Projection"),
        "m_displayMonitorID",
        displayMonitorID);

    std::vector<std::string> enabledOverlays;
    std::string selectedOverlay = m_overlayID;
    const Save::Json & overlaySettings = m_save.section("Overlays");
    const auto enabledIDs = overlaySettings.find("m_enabledOverlayIDs");
    const bool hasMultiOverlaySettings = enabledIDs != overlaySettings.end()
        && enabledIDs->is_array();
    if (hasMultiOverlaySettings)
    {
        for (const Save::Json & id : *enabledIDs)
        {
            if (id.is_string())
            {
                enabledOverlays.push_back(id.get<std::string>());
            }
        }
        Save::read(overlaySettings, "m_selectedOverlayID", selectedOverlay);
    }
    else if (!legacyOverlay.empty() && legacyOverlay != "None")
    {
        enabledOverlays.push_back(legacyOverlay);
        selectedOverlay = legacyOverlay;
    }

    const auto normalizeOverlayID = [](std::string id)
    {
        return id == "Lava Rocks" ? std::string("Balls") : id;
    };
    for (std::string & id : enabledOverlays)
    {
        id = normalizeOverlayID(id);
    }
    selectedOverlay = normalizeOverlayID(selectedOverlay);

    const auto enableMigratedOverlay = [&](const std::string & id)
    {
        if (std::find(enabledOverlays.begin(), enabledOverlays.end(), id)
            == enabledOverlays.end())
        {
            enabledOverlays.push_back(id);
        }
        selectedOverlay = id;
    };

    if (processor == "Minecraft")
    {
        processor = "Blockworld";
    }
    if (processor == "Balls")
    {
        processor = "Colorizer";
        enableMigratedOverlay("Balls");
    }
    if (processor == "WaterFlow")
    {
        processor = "Colorizer";
        enableMigratedOverlay("WaterFlow");
    }
    if (processor == "Vectors")
    {
        processor = "Colorizer";
        enableMigratedOverlay("Vectors");
    }

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
