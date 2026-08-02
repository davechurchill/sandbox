#include "Source_Snapshot.h"
#include "Tools.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include <algorithm>
#include <cfloat>
#include <filesystem>
#include <iostream>

void Source_Snapshot::init()
{
    refreshSnapshotFiles();

    for (int i = 0; i < (int)m_snapshotFiles.size(); i++)
    {
        if (std::filesystem::path(m_snapshotFiles[i].path).filename() == "snapshot.bin")
        {
            m_selectedSnapshot = i;
            break;
        }
    }

    if (m_selectedSnapshot >= 0)
    {
        loadDataDump(m_snapshotFiles[m_selectedSnapshot].path);
    }
}

void Source_Snapshot::imgui()
{
    ImGui::TextUnformatted("Snapshot Files");

    if (ImGui::Button("Refresh"))
    {
        refreshSnapshotFiles();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%d file%s", (int)m_snapshotFiles.size(), m_snapshotFiles.size() == 1 ? "" : "s");

    if (ImGui::Checkbox("Sort by Name", &m_sortByName))
    {
        sortSnapshotFiles();
    }
    if (!m_sortByName)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("Date: newest first");
    }

    if (ImGui::BeginListBox(
        "##SnapshotFiles",
        ImVec2(-FLT_MIN, ImGui::GetTextLineHeightWithSpacing() * 10.0f)))
    {
        if (m_snapshotFiles.empty())
        {
            ImGui::TextDisabled("No snapshot files found.");
        }

        for (int i = 0; i < (int)m_snapshotFiles.size(); i++)
        {
            const std::string name = std::filesystem::path(m_snapshotFiles[i].path).filename().string();
            const bool selected = i == m_selectedSnapshot;
            if (ImGui::Selectable(name.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick))
            {
                m_selectedSnapshot = i;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    loadDataDump(m_snapshotFiles[i].path);
                }
            }
        }

        ImGui::EndListBox();
    }

    const bool hasSelection = m_selectedSnapshot >= 0
        && m_selectedSnapshot < (int)m_snapshotFiles.size();
    ImGui::BeginDisabled(!hasSelection);
    if (ImGui::Button("Load Selected"))
    {
        loadDataDump(m_snapshotFiles[m_selectedSnapshot].path);
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextDisabled("or double-click a file");

    if (!m_loadedSnapshot.empty())
    {
        ImGui::TextWrapped(
            "Loaded: %s",
            std::filesystem::path(m_loadedSnapshot).filename().string().c_str());
    }
    if (!m_loadError.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.30f, 1.0f), "%s", m_loadError.c_str());
    }
}

void Source_Snapshot::render(sf::RenderWindow & window)
{
    window.draw(m_sprite);
}

void Source_Snapshot::refreshSnapshotFiles()
{
    std::string previousSelection;
    if (m_selectedSnapshot >= 0 && m_selectedSnapshot < (int)m_snapshotFiles.size())
    {
        previousSelection = m_snapshotFiles[m_selectedSnapshot].path;
    }

    m_snapshotFiles.clear();
    m_selectedSnapshot = -1;
    m_loadError.clear();

    const std::filesystem::path directory("dataDumps");
    std::error_code error;
    std::filesystem::directory_iterator iterator(directory, error);
    const std::filesystem::directory_iterator end;
    while (!error && iterator != end)
    {
        const std::filesystem::directory_entry & entry = *iterator;
        if (entry.is_regular_file(error) && entry.path().extension() == ".bin")
        {
            std::error_code timeError;
            const std::filesystem::file_time_type modified = entry.last_write_time(timeError);
            m_snapshotFiles.push_back({
                entry.path().string(),
                timeError ? std::filesystem::file_time_type::min() : modified });
        }
        iterator.increment(error);
    }

    if (error)
    {
        m_loadError = "Could not read the dataDumps folder.";
    }

    sortSnapshotFiles();

    const std::string preferred = previousSelection.empty() ? m_loadedSnapshot : previousSelection;
    const auto selected = std::find_if(
        m_snapshotFiles.begin(),
        m_snapshotFiles.end(),
        [&](const SnapshotFile & file) { return file.path == preferred; });
    if (selected != m_snapshotFiles.end())
    {
        m_selectedSnapshot = (int)std::distance(m_snapshotFiles.begin(), selected);
    }
    else if (!m_snapshotFiles.empty())
    {
        m_selectedSnapshot = 0;
    }
}

void Source_Snapshot::sortSnapshotFiles()
{
    std::string selectedPath;
    if (m_selectedSnapshot >= 0 && m_selectedSnapshot < (int)m_snapshotFiles.size())
    {
        selectedPath = m_snapshotFiles[m_selectedSnapshot].path;
    }

    std::sort(m_snapshotFiles.begin(), m_snapshotFiles.end(), [&](const SnapshotFile & left, const SnapshotFile & right)
    {
        const std::string leftName = std::filesystem::path(left.path).filename().string();
        const std::string rightName = std::filesystem::path(right.path).filename().string();
        if (m_sortByName)
        {
            return leftName < rightName;
        }
        if (left.modified != right.modified)
        {
            return left.modified > right.modified;
        }
        return leftName < rightName;
    });

    if (!selectedPath.empty())
    {
        const auto selected = std::find_if(
            m_snapshotFiles.begin(),
            m_snapshotFiles.end(),
            [&](const SnapshotFile & file) { return file.path == selectedPath; });
        m_selectedSnapshot = selected == m_snapshotFiles.end()
            ? -1
            : (int)std::distance(m_snapshotFiles.begin(), selected);
    }
}

bool Source_Snapshot::loadDataDump(const std::string & filename)
{
    try
    {
        cv::FileStorage file(filename, cv::FileStorage::READ);
        if (!file.isOpened())
        {
            m_loadError = "Could not open the selected snapshot.";
            return false;
        }

        cv::Mat snapshot;
        file["matrix"] >> snapshot;
        if (snapshot.empty() || snapshot.channels() != 1)
        {
            m_loadError = "The selected file does not contain a valid terrain snapshot.";
            return false;
        }

        if (snapshot.type() != CV_32F)
        {
            snapshot.convertTo(snapshot, CV_32F);
        }

        m_snapshot = std::move(snapshot);
    }
    catch (const cv::Exception &)
    {
        m_loadError = "The selected snapshot could not be decoded.";
        return false;
    }

    m_image = Tools::matToSfImage(m_snapshot);
    if (!m_texture.loadFromImage(m_image))
    {
        m_loadError = "The selected snapshot could not be uploaded to the graphics card.";
        std::cerr << m_loadError << '\n';
        return false;
    }
    m_sprite.setTexture(m_texture, true);
    m_loadedSnapshot = filename;
    m_loadError.clear();
    markTerrainChanged();
    return true;
}

void Source_Snapshot::save(Settings & save) const
{
    Settings::json & settings = save.section("Source_Snapshot");
    settings["m_loadedSnapshot"] = m_loadedSnapshot;
    settings["m_sortByName"] = m_sortByName;
}

void Source_Snapshot::load(const Settings & save)
{
    const Settings::json & settings = save.section("Source_Snapshot");
    std::string loadedSnapshot;
    Settings::read(settings, "m_loadedSnapshot", loadedSnapshot);
    Settings::read(settings, "m_sortByName", m_sortByName);
    sortSnapshotFiles();
    if (!loadedSnapshot.empty())
    {
        loadDataDump(loadedSnapshot);
    }
}

cv::Mat Source_Snapshot::getTopography()
{
    return m_snapshot;
}
