#pragma once

#include<curlpp/Easy.hpp>
#include<curlpp/cURLpp.hpp>
#include<curlpp/Options.hpp>

#include "MinecraftInterface.h"

#include <format>
#include <sstream>
#include <imgui.h>

MinecraftInterface::MinecraftInterface()
{
    setup();
}

void MinecraftInterface::setup()
{
    m_handle.setOpt(curlpp::options::Port(9000));
}

void MinecraftInterface::command(const std::string & command, int x, int y, int z, Dimension dimension)
{
    curlpp::Cleanup clean;
    try
    {
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

void MinecraftInterface::projectHeightmap(const Grid<float> & heightMap, int blockScale, float waterLevel, int x, int y, int z)
{
    int wx = heightMap.width();
    int wz = heightMap.height();

    if (wx <= 0 || wz <= 0) { return; }

    int water = std::ceil(blockScale * waterLevel);
    std::cout << waterLevel << std::endl;
    // Clear space
    fill(x - 1, y - 1, z - 1, x + wx, y + blockScale, z + wz, "minecraft:spruce_planks", "outline");
    fill(x, y, z, x + wx - 1, y + water, z + wz - 1, "minecraft:water");
    fill(x, y + water + 1, z, x + wx - 1, y + blockScale, z + wz - 1, "minecraft:air");

    BlockPlacer placer;
    for (size_t i = 0; i < wx; ++i)
    {
        for (size_t j = 0; j < wz; ++j)
        {
            int height = heightMap.get(i, j) * blockScale;
            int tx = i + x;
            int tz = j + z;
            if (height >= water)
            {
                placer.addBlocks(tx, y + height, tz, tx, y + height, tz, "minecraft:grass_block");
                height--;
            }
            if (height > 1)
            {
                placer.addBlocks(tx, y + height, tz, tx, y + height, tz, "minecraft:dirt");
                height--;
            }
            if (height > 1)
            {
                placer.addBlocks(tx, y, tz, tx, y + height, tz, "minecraft:stone");
            }
        }
    }

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

void MinecraftInterface::imgui(const Grid<float> grid, float waterLevel)
{
    if (ImGui::Button("Test Connection"))
    {
        command("say testing testing");
    }

    ImGui::Text("Size: %d, %d, %d", grid.width(), m_mcHeight, grid.height());
    ImGui::SliderInt("Block Height", &m_mcHeight, 10, 100);

    if (ImGui::Button("Project Perlin Noise"))
    {
        projectHeightmap(grid, m_mcHeight, waterLevel / 255.0);
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
