#pragma once

#include "Settings.hpp"
#include "TopographyOverlay.hpp"
#include "TopographyProcessor.hpp"
#include "TopographySource.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

class SandboxSession
{
    struct OverlayState
    {
        std::shared_ptr<TopographyOverlay> overlay;
        bool enabled = false;
    };

    cv::Mat m_topography;
    Settings m_settings;
    SandBoxProjector m_projector;

    std::string m_sourceID = "Camera";
    std::string m_processorID = "Colorizer";
    std::string m_overlayID = "Animals";

    std::shared_ptr<TopographySource> m_source;
    std::shared_ptr<TopographyProcessor> m_processor;

    std::map<std::string, OverlayState> m_overlayStates;

    OverlayState * ensureOverlay(const std::string & name);

public:
    SandboxSession();

    void processFrame(float deltaTime);

    TopographySource * source() const { return m_source.get(); }
    TopographyProcessor * processor() const { return m_processor.get(); }
    TopographyOverlay * overlay() const;
    TopographyOverlay * inputOverlay() const;
    bool overlayEnabled(const std::string & name) const;
    SandBoxProjector & projector() { return m_projector; }
    const cv::Mat & topography() const { return m_topography; }

    const std::string & sourceID() const { return m_sourceID; }
    const std::string & processorID() const { return m_processorID; }
    const std::string & overlayID() const { return m_overlayID; }

    std::vector<std::string> sourceNames() const;
    std::vector<std::string> processorNames() const;
    std::vector<std::string> overlayNames() const;

    void setSource(const std::string & source, bool saveCurrent = true);
    void setProcessor(const std::string & processor, bool saveCurrent = true);
    void setOverlay(const std::string & overlay, bool saveCurrent = true);
    void setOverlayEnabled(const std::string & overlay, bool enabled);
    void renderOverlays(sf::RenderWindow & window);

    void saveSettings(const std::string & filename, bool doubleSizeUI, const std::string & displayMonitorID);
    bool loadSettings(const std::string & filename, bool & doubleSizeUI, std::string & displayMonitorID);
};
