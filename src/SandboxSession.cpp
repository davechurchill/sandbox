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
    m_overlayMap.emplace("None", []() { return nullptr; });
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
        if (m_overlay)
        {
            m_overlay->processTopographyOverlay(data, *m_processor);
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
    if (saveCurrent && m_overlay) { m_overlay->saveOverlay(m_save); }
    m_overlayID = overlay;
    if (m_overlayMap.contains(overlay))
    {
        m_overlay = m_overlayMap.at(overlay)();
    }
    else
    {
        m_overlayID = "None";
        m_overlay = m_overlayMap.at("None")();
    }
    if (m_overlay)
    {
        m_overlay->initOverlay();
        m_overlay->loadOverlay(m_save);
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
    if (m_overlay) { m_overlay->saveOverlay(m_save); }

    Save::Json & settings = m_save.section("Scene_Main");
    settings["m_sourceID"] = m_sourceID;
    settings["m_processorID"] = m_processorID;
    settings["m_overlayID"] = m_overlayID;
    settings["m_doubleSizeUI"] = doubleSizeUI;
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
    std::string overlay = m_overlayID;
    Save::read(settings, "m_sourceID", source);
    Save::read(settings, "m_processorID", processor);
    Save::read(settings, "m_overlayID", overlay);
    Save::read(settings, "m_doubleSizeUI", doubleSizeUI);
    Save::read(
        m_save.section("Projection"),
        "m_displayMonitorID",
        displayMonitorID);

    if (processor == "Minecraft")
    {
        processor = "Blockworld";
    }
    if (processor == "Balls")
    {
        processor = "Colorizer";
        overlay = "Balls";
    }
    if (processor == "WaterFlow")
    {
        processor = "Colorizer";
        overlay = "WaterFlow";
    }
    if (processor == "Vectors")
    {
        processor = "Colorizer";
        overlay = "Vectors";
    }
    if (overlay == "Lava Rocks")
    {
        overlay = "Balls";
    }

    setSource(source, false);
    setProcessor(processor, false);
    setOverlay(overlay, false);
    return true;
}
