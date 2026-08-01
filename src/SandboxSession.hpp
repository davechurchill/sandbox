#pragma once

#include "Save.hpp"
#include "TopographyOverlay.hpp"
#include "TopographyProcessor.hpp"
#include "TopographySource.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

class SandboxSession
{
    using SourceFactory = std::function<std::shared_ptr<TopographySource>()>;
    using ProcessorFactory = std::function<std::shared_ptr<TopographyProcessor>()>;
    using OverlayFactory = std::function<std::shared_ptr<TopographyOverlay>()>;

    cv::Mat m_topography;
    Save m_save;

    std::string m_sourceID = "Camera";
    std::string m_processorID = "Colorizer";
    std::string m_overlayID = "None";

    std::shared_ptr<TopographySource> m_source;
    std::shared_ptr<TopographyProcessor> m_processor;
    std::shared_ptr<TopographyOverlay> m_overlay;

    std::map<std::string, SourceFactory> m_sourceMap;
    std::map<std::string, ProcessorFactory> m_processorMap;
    std::map<std::string, OverlayFactory> m_overlayMap;

    template <class T>
        requires (std::is_base_of_v<TopographySource, T>
            && !std::is_base_of_v<TopographyProcessor, T>
            && !std::is_base_of_v<TopographyOverlay, T>)
    void registerSource(const std::string & name)
    {
        m_sourceMap.emplace(name, std::make_shared<T>);
    }

    template <class T>
        requires (std::is_base_of_v<TopographyProcessor, T>
            && !std::is_base_of_v<TopographyOverlay, T>)
    void registerProcessor(const std::string & name)
    {
        m_processorMap.emplace(name, std::make_shared<T>);
    }

    template <class T>
        requires (std::is_base_of_v<TopographyOverlay, T>
            && !std::is_base_of_v<TopographyProcessor, T>)
    void registerOverlay(const std::string & name)
    {
        m_overlayMap.emplace(name, std::make_shared<T>);
    }

    void registerComponents();

public:
    SandboxSession();

    void processFrame(float deltaTime);

    TopographySource * source() const { return m_source.get(); }
    TopographyProcessor * processor() const { return m_processor.get(); }
    TopographyOverlay * overlay() const { return m_overlay.get(); }
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

    void saveSettings(
        const std::string & filename,
        bool doubleSizeUI,
        const std::string & displayMonitorID);
    bool loadSettings(
        const std::string & filename,
        bool & doubleSizeUI,
        std::string & displayMonitorID);
};
