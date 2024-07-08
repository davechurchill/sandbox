#pragma once

#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include "Cube.hpp"

namespace mc
{
    class GenerationProfile
    {
    protected:
        std::vector<std::string> m_blockNames;
    public:
        virtual void imgui() = 0;
        virtual void generate(Cube<uint8_t> & output, const cv::Mat & input, int blockScale) = 0;
        inline const std::string & blockName(uint8_t id) const { return m_blockNames[id]; };
    };

    class BasicGrassProfile : public GenerationProfile
    {
        float m_waterLevel = 0.5f;
    public:
        BasicGrassProfile();
        void imgui();
        void generate(Cube<uint8_t> & output, const cv::Mat & input, int blockScale);
    };

    class MonochromeProfile : public GenerationProfile
    {
    public:
        MonochromeProfile();
        void imgui();
        void generate(Cube<uint8_t> & output, const cv::Mat & input, int blockScale);
    };
}
