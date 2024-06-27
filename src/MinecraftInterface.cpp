#include "MinecraftInterface.h"

#include <format>
#include <sstream>
#include <imgui.h>

#ifdef Use_Minecraft
#include<curlpp/Easy.hpp>
#include<curlpp/cURLpp.hpp>
#include<curlpp/Options.hpp>

using namespace mc;

MinecraftInterface::MinecraftInterface()
{
    setup();
    m_profile = std::make_shared<MonochromeProfile>();
}

void MinecraftInterface::setup()
{
    m_handle.setOpt(curlpp::options::Port(9000));
}

void MinecraftInterface::command(const std::string & command, int x, int y, int z, Dimension dimension)
{
    try
    {
        curlpp::Cleanup clean;
        std::string dim = "overworld";
        if (dimension == Dimension::End)
        {
            dim = "end";
        }
        else if (dimension == Dimension::Nether)
        {
            dim = "nether";
        }
        m_handle.setOpt(curlpp::options::Url(std::format("http://localhost/commands?x={}&y={}&z={}&dimension={}", x, y, z, dim)));
        std::list<std::string> header;
        header.push_back("Content-Type: text/plain; charset=UTF-8");
        m_handle.setOpt(curlpp::options::HttpHeader(header));
        m_handle.setOpt(curlpp::options::PostFields(command));
        m_handle.setOpt(curlpp::options::PostFieldSize(command.length()));
        std::cout << m_handle << std::endl;
    }
    catch (curlpp::LibcurlRuntimeError e) {
        std::cout << "Exception: " << e.what() << std::endl;
        throw e;
    }
}

void MinecraftInterface::projectHeightmap(const Grid<float> & heightMap, int blockScale)
{
    int nextCubeId = (m_currentCube + 1) % 2;
    m_profile->generate(m_cubes[nextCubeId], heightMap, blockScale);
    
    int wx = heightMap.width();
    int wz = heightMap.height();
    // Clear space
    fill(m_x - 1, m_y - 1, m_z - 1, m_x + wx, m_y + blockScale, m_z + wz, "minecraft:spruce_planks", "outline");
    fill(m_x, m_y, m_z, m_x + wx - 1, m_y + blockScale, m_z + wz - 1, "minecraft:air");

    const Cube<uint8_t> & cube = m_cubes[nextCubeId];
    BlockPlacer placer;
    for (size_t x = 0; x < wx; ++x)
    {
        for (size_t y = 0; y < blockScale; ++y)
        {
            for (size_t z = 0; z < wz; ++z)
            {
                uint8_t block = cube.get(x, y, z);
                if (block != 0)
                {  
                    placer.addBlock(x + m_x, y + m_y, z + m_z, m_profile->blockName(block));
                }
            }
        }
    }

    m_currentCube = nextCubeId;
    std::cout << "Sending Blocks" << std::endl;
    placer.send(m_handle);
}

void MinecraftInterface::projectHeightmapChanges(const Grid<float> & heightMap, int blockScale)
{
    int nextCubeId = (m_currentCube + 1) % 2;
    m_profile->generate(m_cubes[nextCubeId], heightMap, blockScale);

    int wx = heightMap.width();
    int wz = heightMap.height();

    const Cube<uint8_t> & cube = m_cubes[nextCubeId];
    const Cube<uint8_t> & pastCube = m_cubes[m_currentCube];
    BlockPlacer placer;
    for (size_t x = 0; x < wx; ++x)
    {
        for (size_t y = 0; y < blockScale; ++y)
        {
            for (size_t z = 0; z < wz; ++z)
            {
                uint8_t block = cube.get(x, y, z);
                if (block != pastCube.get(x,y,z))
                {
                    placer.addBlock(x + m_x, y + m_y, z + m_z, m_profile->blockName(block));
                }
            }
        }
    }

    m_currentCube = nextCubeId;
    std::cout << "Sending Blocks" << std::endl;
    placer.send(m_handle);
}

inline void MinecraftInterface::fill(int x1, int y1, int z1, int x2, int y2, int z2, const std::string & block, const std::string & args)
{
    if (args == "")
    {
        command(std::format("fill {} {} {} {} {} {} {}", x1, y1, z1, x2, y2, z2, block));
    }
    else
    {
        command(std::format("fill {} {} {} {} {} {} {} {}", x1, y1, z1, x2, y2, z2, block, args));
    }
}

void MinecraftInterface::imgui(const Grid<float> & grid)
{
    if (ImGui::Button("Test Connection"))
    {
        command("say testing testing");
    }

    ImGui::Text("Size: %d, %d, %d", grid.width(), m_mcHeight, grid.height());
    ImGui::SliderInt("Block Height", &m_mcHeight, 10, 100);

    if (ImGui::Button("Project Perlin Noise"))
    {
        projectHeightmap(grid, m_mcHeight);
    }

    if (ImGui::Button("Update Perlin Noise"))
    {
        projectHeightmapChanges(grid, m_mcHeight);
    }

    if (ImGui::CollapsingHeader("Generation Profile Settings"))
    {
        m_profile->imgui();
    }
}

MinecraftInterface::BlockPlacer::BlockPlacer()
{
    m_blocks << "[";
}

void MinecraftInterface::BlockPlacer::addBlock(int x, int y, int z, const std::string & name)
{
    m_blocks << "{\"id\": \"" << name << "\", \"x\":" << x << ", \"y\": " << y << ", \"z\": " << z << "},";
}

void MinecraftInterface::BlockPlacer::addBlocks(int x1, int y1, int z1, int x2, int y2, int z2, const std::string & name)
{
    int xstep = (x2 - x1 >= 0) ? 1 : -1;
    int ystep = (y2 - y1 >= 0) ? 1 : -1;
    int zstep = (z2 - z1 >= 0) ? 1 : -1;

    for (int x = x1; x <= x2; x += xstep)
    {
        for (int y = y1; y <= y2; y += ystep)
        {
            for (int z = z1; z <= z2; z += zstep)
            {
                addBlock(x, y, z, name);
            }
        }
    }
    
}

void MinecraftInterface::BlockPlacer::send(curlpp::Easy & m_handle)
{
    // remove last comma and close list
    std::string total = m_blocks.str();
    total.pop_back();
    total.push_back(']');
    curlpp::Cleanup clean;
    try
    {
        m_handle.setOpt(curlpp::options::Url("http://localhost/blocks"));
        std::list<std::string> header;
        header.push_back("Content-Type: application/json");
        m_handle.setOpt(curlpp::options::HttpHeader(header));
        m_handle.setOpt(curlpp::options::CustomRequest("PUT"));
        m_handle.setOpt(curlpp::options::PostFields(total));
        m_handle.setOpt(curlpp::options::PostFieldSize(total.length()));
        std::cout << m_handle << std::endl;
    }
    catch (curlpp::LibcurlRuntimeError e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
        throw e;
    }
}

#else
MinecraftInterface::MinecraftInterface(){}

void MinecraftInterface::imgui(const Grid<float> & grid, float waterLevel)
{
    ImGui::Text("Minecraft connection not compiled, please define Use_Minecraft in MinecraftInterface.h");
}
#endif