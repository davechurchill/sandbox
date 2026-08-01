#pragma once

#include "Save.hpp"
#include "TopographySource.h"

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include <filesystem>
#include <string>
#include <vector>

class Source_Snapshot : public TopographySource
{
    struct SnapshotFile
    {
        std::string path;
        std::filesystem::file_time_type modified;
    };

    cv::Mat m_snapshot;

    sf::Image m_image;
    sf::Texture m_texture;
    sf::Sprite m_sprite{ m_texture };

    std::vector<SnapshotFile> m_snapshotFiles;
    std::string m_loadedSnapshot;
    std::string m_loadError;
    int m_selectedSnapshot = -1;
    bool m_sortByName = false;

    void refreshSnapshotFiles();
    void sortSnapshotFiles();
    bool loadDataDump(const std::string & filename);
public:
    void init();
    void imgui();
    void render(sf::RenderWindow & window);
    void processEvent(const sf::Event & event, const sf::Vector2f & mouse);
    void save(Save & save) const;
    void load(const Save & save);

    cv::Mat getTopography();
};
