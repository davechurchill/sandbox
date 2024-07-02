#pragma once

//#define Use_Minecraft
#ifdef Use_Minecraft
#include <curlpp/Easy.hpp>
#include <curlpp/cURLpp.hpp>
#endif

#include <sstream>
#include "Grid.hpp"
#include "Cube.hpp"
#include "BlockGeneration.h"

namespace mc
{
    class MinecraftInterface
    {
#ifdef Use_Minecraft
        curlpp::Easy m_handle;
        curlpp::Cleanup m_clean;

        int m_mcHeight = 30;

        class BlockPlacer
        {
            std::stringstream m_blocks;
        public:

            BlockPlacer();

            void addBlock(int x, int y, int z, const std::string & name);
            void addBlocks(int x1, int y1, int z1, int x2, int y2, int z2, const std::string & name);
            void send(curlpp::Easy & handle);
        };
#endif // Use_Minecraft
        int m_currentCube = 0;
        int m_x = 0;
        int m_y = 0;
        int m_z = 0;
        Cube<uint8_t> m_cubes[2];

        std::shared_ptr <GenerationProfile> m_profile;

    public:

        enum class Dimension
        {
            Overworld,
            End,
            Nether
        };

        MinecraftInterface();

        void setup();

        void command(const std::string & command, int x = 0, int y = 0, int z = 0, Dimension dimension = Dimension::Overworld);

        void projectHeightmap(const Grid<float> & heightMap, int blockScale);
        void projectHeightmapChanges(const Grid<float> & heightMap, int blockScale);

        inline void setCoords(int x = 0, int y = 0, int z = 0) { m_x = x; m_y = y; m_z = z; }

        inline void fill(int x1, int y1, int z1, int x2, int y2, int z2, const std::string & block, const std::string & args = "");

        void imgui(const Grid<float> & grid);
    };
}