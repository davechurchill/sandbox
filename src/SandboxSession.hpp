#pragma once

#include "Settings.hpp"
#include "TerrainContext.hpp"
#include "TopographySource.hpp"
#include "TopographyVisualizer.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class SandboxSession
{
    struct VisualizerState
    {
        std::unique_ptr<TopographyVisualizer> visualizer;
        bool enabled = false;
    };

    cv::Mat m_topography;
    std::uint64_t m_terrainRevision = 0;
    std::uint64_t m_sourceRevision = 0;
    Settings m_settings;
    SandBoxProjector m_projector;
    TerrainContext m_terrainContext{ m_projector };

    std::string m_sourceName = "Camera";
    std::string m_visualizerName = "Colorizer";

    std::unique_ptr<TopographySource> m_source;
    std::map<std::string, VisualizerState, std::less<>> m_visualizerStates;

    VisualizerState * ensureVisualizer(std::string_view name);

public:
    SandboxSession();

    void processFrame(float deltaTime);

    TopographySource & source() { return *m_source; }
    const TopographySource & source() const { return *m_source; }
    TopographyVisualizer * visualizer() const;
    TopographyVisualizer * inputVisualizer() const;
    bool visualizerEnabled(std::string_view name) const;
    SandBoxProjector & projector() { return m_projector; }
    const cv::Mat & topography() const { return m_topography; }

    const std::string & sourceName() const { return m_sourceName; }
    const std::string & visualizerName() const { return m_visualizerName; }

    const std::vector<std::string> & sourceNames() const;
    const std::vector<std::string> & visualizerNames() const;

    void setSource(std::string_view source, bool saveCurrent = true);
    void setVisualizer(std::string_view visualizer, bool saveCurrent = true);
    void setVisualizerEnabled(std::string_view visualizer, bool enabled);
    void renderVisualizers(sf::RenderWindow & window);

    void saveSettings(const std::string & filename, bool doubleSizeUI, const std::string & displayMonitorID);
    bool loadSettings(const std::string & filename, bool & doubleSizeUI, std::string & displayMonitorID);
};
