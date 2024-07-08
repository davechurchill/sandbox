#include "BlockGeneration.h"
#include <imgui-SFML.h>
#include <imgui.h>

namespace mc
{
    BasicGrassProfile::BasicGrassProfile()
    {
        m_blockNames = { "minecraft:air", "minecraft:stone", "minecraft:dirt", "minecraft:grass_block", "minecraft:water"};
    }

    void BasicGrassProfile::imgui()
    {
        ImGui::SliderFloat("MC Water Level", &m_waterLevel, 0.0f, 1.0f);
    }

    void BasicGrassProfile::generate(Cube<uint8_t> & output, const cv::Mat & input, int blockScale)
    {
        int wx = input.cols;
        int wz = input.rows;
        int water = (int)(std::ceil(blockScale * m_waterLevel));

        if (wx <= 0 || wz <= 0) { return; }

        output.refill(wx, blockScale, wz, 0);

        for (int i = 0; i < wx; ++i)
        {
            for (int j = 0; j < wz; ++j)
            {
                int height = (int)(input.at<float>(j, i) * blockScale);
                if (height >= water)
                {
                    output.fill(i, height, j, i, height, j, 3);
                    height--;
                }
                else
                {
                    output.fill(i, height, j, i, water, j, 4);
                }
                if (height > 1)
                {
                    output.fill(i, height, j, i, height, j, 2);
                    height--;
                }
                if (height > 1)
                {
                    output.fill(i, 0, j, i, height, j, 1);
                }
            }
        }
    }

    MonochromeProfile::MonochromeProfile()
    {
        m_blockNames = { "minecraft:air", "minecraft:black_concrete", "minecraft:gray_concrete", "minecraft:light_gray_concrete", "minecraft:white_concrete"};
    }
    void MonochromeProfile::imgui()
    {
    }
    void MonochromeProfile::generate(Cube<uint8_t> & output, const cv::Mat & input, int blockScale)
    {
        int wx = input.cols;
        int wz = input.rows;

        if (wx <= 0 || wz <= 0) { return; }

        output.refill(wx, blockScale, wz, 0);

        for (int i = 0; i < wx; ++i)
        {
            for (int j = 0; j < wz; ++j)
            {
                int height = (int)(input.at<float>(j, i) * blockScale);
                output.fill(i, 0, j, i, height, j, height * 4 / blockScale + 1);
            }
        }
    }
}