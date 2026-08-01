#include "SandboxSession.hpp"

#include "Visualizer_AStar.h"
#include "Visualizer_Animals.h"
#include "Visualizer_BFS.h"
#include "Visualizer_Balls.h"
#include "Visualizer_Cloth.h"
#include "Visualizer_ColorAdjustment.h"
#include "Visualizer_ContourLines.h"
#include "Visualizer_SmokeFire.h"
#include "Visualizer_Vectors.h"
#include "Visualizer_WaterFlow.h"
#include "Visualizer_Weather.h"
#include "Visualizer_Blockworld.h"
#include "Visualizer_Colorizer.h"
#include "Visualizer_FishPond.h"
#include "Visualizer_ForestFire.h"
#include "Visualizer_Heat.h"
#include "Visualizer_Nature.h"
#include "Visualizer_TerrainLighting.h"
#include "Source_Camera.h"
#include "Source_PaintBrush.h"
#include "Source_Perlin.h"
#include "Source_Snapshot.h"
#include "Source_Waves.h"

namespace
{
    std::unique_ptr<TopographySource> createSource(std::string_view name)
    {
        if (name == "Camera") return std::make_unique<Source_Camera>();
        if (name == "PaintBrush") return std::make_unique<Source_PaintBrush>();
        if (name == "Perlin") return std::make_unique<Source_Perlin>();
        if (name == "Snapshot") return std::make_unique<Source_Snapshot>();
        if (name == "Waves") return std::make_unique<Source_Waves>();
        return nullptr;
    }

    std::unique_ptr<TopographyVisualizer> createVisualizer(std::string_view name)
    {
        if (name == Visualizer_Blockworld::Name) return std::make_unique<Visualizer_Blockworld>();
        if (name == Visualizer_Colorizer::Name) return std::make_unique<Visualizer_Colorizer>();
        if (name == Visualizer_FishPond::Name) return std::make_unique<Visualizer_FishPond>();
        if (name == Visualizer_ForestFire::Name) return std::make_unique<Visualizer_ForestFire>();
        if (name == Visualizer_Heat::Name) return std::make_unique<Visualizer_Heat>();
        if (name == Visualizer_Nature::Name) return std::make_unique<Visualizer_Nature>();
        if (name == Visualizer_TerrainLighting::Name) return std::make_unique<Visualizer_TerrainLighting>();
        if (name == Visualizer_ColorAdjustment::Name) return std::make_unique<Visualizer_ColorAdjustment>();
        if (name == Visualizer_Animals::Name) return std::make_unique<Visualizer_Animals>();
        if (name == Visualizer_BFS::Name) return std::make_unique<Visualizer_BFS>();
        if (name == Visualizer_Balls::Name) return std::make_unique<Visualizer_Balls>();
        if (name == Visualizer_Cloth::Name) return std::make_unique<Visualizer_Cloth>();
        if (name == Visualizer_ContourLines::Name) return std::make_unique<Visualizer_ContourLines>();
        if (name == Visualizer_AStar::Name) return std::make_unique<Visualizer_AStar>();
        if (name == Visualizer_SmokeFire::Name) return std::make_unique<Visualizer_SmokeFire>();
        if (name == Visualizer_Vectors::Name) return std::make_unique<Visualizer_Vectors>();
        if (name == Visualizer_WaterFlow::Name) return std::make_unique<Visualizer_WaterFlow>();
        if (name == Visualizer_Weather::Name) return std::make_unique<Visualizer_Weather>();
        return nullptr;
    }
}

SandboxSession::SandboxSession()
{
    setSource(m_sourceName, false);
    setVisualizer(m_visualizerName, false);
    setVisualizerEnabled(m_visualizerName, true);
}

void SandboxSession::processFrame(float deltaTime)
{
    m_topography = m_source->getTopography();
    if (m_sourceRevision != m_source->revision())
    {
        m_sourceRevision = m_source->revision();
        ++m_terrainRevision;
    }
    if (m_topography.empty())
    {
        return;
    }

    const std::vector<MarkerData> markers = m_source->getMarkers();
    const TerrainFrame frame { m_topography, deltaTime, markers, m_terrainRevision };
    for (const std::string & name : visualizerNames())
    {
        const auto found = m_visualizerStates.find(name);
        if (found != m_visualizerStates.end() && found->second.enabled && found->second.visualizer)
        {
            found->second.visualizer->process(frame);
        }
    }
}

const std::vector<std::string> & SandboxSession::sourceNames() const
{
    static const std::vector<std::string> names { "Camera", "PaintBrush", "Perlin", "Snapshot", "Waves" };
    return names;
}

const std::vector<std::string> & SandboxSession::visualizerNames() const
{
    static const std::vector<std::string> names{
        std::string(Visualizer_Blockworld::Name),
        std::string(Visualizer_Colorizer::Name),
        std::string(Visualizer_FishPond::Name),
        std::string(Visualizer_ForestFire::Name),
        std::string(Visualizer_Heat::Name),
        std::string(Visualizer_Nature::Name),
        std::string(Visualizer_TerrainLighting::Name),
        std::string(Visualizer_ColorAdjustment::Name),
        std::string(Visualizer_Animals::Name),
        std::string(Visualizer_BFS::Name),
        std::string(Visualizer_Balls::Name),
        std::string(Visualizer_Cloth::Name),
        std::string(Visualizer_ContourLines::Name),
        std::string(Visualizer_AStar::Name),
        std::string(Visualizer_SmokeFire::Name),
        std::string(Visualizer_Vectors::Name),
        std::string(Visualizer_WaterFlow::Name),
        std::string(Visualizer_Weather::Name)
    };
    return names;
}

SandboxSession::VisualizerState * SandboxSession::ensureVisualizer(std::string_view name)
{
    const auto found = m_visualizerStates.find(name);
    if (found != m_visualizerStates.end())
    {
        return found->second.visualizer ? &found->second : nullptr;
    }

    std::unique_ptr<TopographyVisualizer> visualizer = createVisualizer(name);
    if (!visualizer)
    {
        return nullptr;
    }

    visualizer->setContext(m_terrainContext);
    visualizer->init();
    visualizer->load(m_settings);
    const std::string visualizerName(visualizer->name());
    const auto inserted = m_visualizerStates.emplace(
        visualizerName,
        VisualizerState{ std::move(visualizer), false });
    return &inserted.first->second;
}

TopographyVisualizer * SandboxSession::visualizer() const
{
    const auto found = m_visualizerStates.find(m_visualizerName);
    return found != m_visualizerStates.end()
        ? found->second.visualizer.get()
        : nullptr;
}

TopographyVisualizer * SandboxSession::inputVisualizer() const
{
    const auto found = m_visualizerStates.find(m_visualizerName);
    return found != m_visualizerStates.end()
        && found->second.enabled
        ? found->second.visualizer.get()
        : nullptr;
}

bool SandboxSession::visualizerEnabled(std::string_view name) const
{
    const auto found = m_visualizerStates.find(name);
    return found != m_visualizerStates.end() && found->second.enabled;
}

void SandboxSession::setSource(std::string_view source, bool saveCurrent)
{
    std::string selectedName(source);
    std::unique_ptr<TopographySource> nextSource = createSource(selectedName);
    if (!nextSource)
    {
        selectedName = "Camera";
        nextSource = createSource(selectedName);
    }

    const bool sourceChanged = !m_source || selectedName != m_sourceName;
    if (m_source)
    {
        if (saveCurrent) { m_source->save(m_settings); }
        m_source->deactivate();
    }
    m_sourceName = std::move(selectedName);
    m_source = std::move(nextSource);
    m_source->init();
    m_source->load(m_settings);
    m_source->activate();
    m_sourceRevision = m_source->revision();
    ++m_terrainRevision;

    if (sourceChanged)
    {
        for (auto & [_, state] : m_visualizerStates)
        {
            if (state.visualizer)
            {
                state.visualizer->onSourceChanged();
            }
        }
    }
}

void SandboxSession::setVisualizer(std::string_view visualizer, bool saveCurrent)
{
    if (saveCurrent)
    {
        if (TopographyVisualizer * current = this->visualizer())
        {
            current->save(m_settings);
        }
    }

    if (visualizer.empty())
    {
        m_visualizerName.clear();
        return;
    }

    std::string selectedName(visualizer);
    VisualizerState * state = ensureVisualizer(selectedName);
    if (!state)
    {
        selectedName = std::string(Visualizer_Colorizer::Name);
        state = ensureVisualizer(selectedName);
    }
    if (state)
    {
        m_visualizerName = state->visualizer->name();
    }
}

void SandboxSession::setVisualizerEnabled(std::string_view visualizer, bool enabled)
{
    VisualizerState * state = ensureVisualizer(visualizer);
    if (state && state->enabled != enabled)
    {
        if (enabled) { state->visualizer->activate(); }
        else { state->visualizer->deactivate(); }
        state->enabled = enabled;
        refreshWalkabilityProvider();
    }
}

void SandboxSession::refreshWalkabilityProvider()
{
    const TopographyVisualizer * provider = nullptr;
    for (const std::string & name : visualizerNames())
    {
        const auto found = m_visualizerStates.find(name);
        if (found != m_visualizerStates.end()
            && found->second.enabled
            && found->second.visualizer
            && found->second.visualizer->definesTerrainWalkability())
        {
            provider = found->second.visualizer.get();
        }
    }
    if (provider)
    {
        m_terrainContext.setWalkabilityProvider(
            [provider](const cv::Mat & terrain, const cv::Point2f & position)
            {
                return provider->isTerrainWalkable(terrain, position);
            });
    }
    else
    {
        m_terrainContext.setWalkabilityProvider({});
    }
}

void SandboxSession::renderVisualizers(sf::RenderWindow & window)
{
    for (const std::string & name : visualizerNames())
    {
        const auto found = m_visualizerStates.find(name);
        if (found != m_visualizerStates.end()
            && found->second.enabled
            && found->second.visualizer)
        {
            found->second.visualizer->render(window);
        }
    }
}

void SandboxSession::saveSettings(const std::string & filename, bool doubleSizeUI, const std::string & displayMonitorID)
{
    m_settings.setCurrentSchemaVersion();
    m_projector.save(m_settings);
    m_source->save(m_settings);
    for (const auto & [_, state] : m_visualizerStates)
    {
        if (state.visualizer) { state.visualizer->save(m_settings); }
    }

    Settings::json & guiSettings = m_settings.section("SandboxGUI");
    guiSettings["m_sourceName"] = m_sourceName;
    guiSettings["m_doubleSizeUI"] = doubleSizeUI;

    Settings::json & visualizerSettings = m_settings.section("Visualizers");
    visualizerSettings["m_enabledVisualizerNames"] = Settings::json::array();
    for (const std::string & name : visualizerNames())
    {
        const auto found = m_visualizerStates.find(name);
        if (found != m_visualizerStates.end() && found->second.enabled)
        {
            visualizerSettings["m_enabledVisualizerNames"].push_back(name);
        }
    }
    visualizerSettings["m_selectedVisualizerName"] = m_visualizerName;

    m_settings.section("Projection")["m_displayMonitorID"] = displayMonitorID;
    m_settings.saveToFile(filename);
}

bool SandboxSession::loadSettings(const std::string & filename, bool & doubleSizeUI, std::string & displayMonitorID)
{
    if (!m_settings.loadFromFile(filename))
    {
        return false;
    }

    m_projector.load(m_settings);

    const Settings::json & guiSettings = m_settings.section("SandboxGUI");
    std::string source(m_sourceName);
    Settings::read(guiSettings, "m_sourceName", source);
    Settings::read(guiSettings, "m_doubleSizeUI", doubleSizeUI);
    Settings::read(m_settings.section("Projection"), "m_displayMonitorID", displayMonitorID);

    std::vector<std::string> enabledVisualizers;
    std::string selectedVisualizer(m_visualizerName);
    const Settings::json & visualizerSettings = m_settings.section("Visualizers");
    const auto enabledIDs = visualizerSettings.find("m_enabledVisualizerNames");
    if (enabledIDs != visualizerSettings.end() && enabledIDs->is_array())
    {
        for (const Settings::json & id : *enabledIDs)
        {
            if (id.is_string())
            {
                enabledVisualizers.push_back(id.get<std::string>());
            }
        }
    }
    Settings::read(visualizerSettings, "m_selectedVisualizerName", selectedVisualizer);

    setSource(source, false);
    for (auto & [_, state] : m_visualizerStates)
    {
        if (state.enabled && state.visualizer)
        {
            state.visualizer->deactivate();
        }
    }
    m_terrainContext.setWalkabilityProvider({});
    m_visualizerStates.clear();

    for (const std::string & id : enabledVisualizers)
    {
        setVisualizerEnabled(id, true);
    }
    setVisualizer(selectedVisualizer, false);
    return true;
}
