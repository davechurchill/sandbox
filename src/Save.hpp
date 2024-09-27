#pragma once

#include <fstream>
#include <opencv2/opencv.hpp>

struct Save
{
    // main
    std::string source = "Camera";
    std::string processor = "Colorizer";

    // camera
    int align = 0;
    bool gaussianBlur = true;
    float maxDistance = 1.13f;
    float minDistance = 0.90f;
    bool drawDepth = true;
    bool drawColor = false;
    int fpsSetting = 0;

    // perlin
    int octaves = 5;
    int seed = 0;
    int seedSize = 9;
    float persistance = 0.5f;
    bool drawGrid = false;

    // filters
    float temporalAlpha = 0.047f;
    int temporalDelta = 72;
    int temporalPersistance = 3;
    int holeFill = 1;

    // data warper
    cv::Point2f warpPoints[4] = { {100, 100}, {200, 100}, {100, 200}, {200, 200} };
    bool applyHeightAdjustment = false;
    cv::Point2f planarPoints[3] = { {150, 0}, {75, 150}, {0, 75} };

    //sandbox projector
    cv::Point2f projectionPoints[4] = { {400, 400}, {500, 400}, {400, 500}, {500, 500} };
    bool drawLines = true;

    // Colorizer
    int selectedShaderIndex = 0;
    bool drawContours = true;
    int numberOfContourLines = 15;
    bool drawProjection = true;


    void saveToFile(const std::string & filename)
    {
        std::ofstream fout(filename);

        if(!fout.good()) 
        {
            std::cout << "Failed to open file: " << filename << std::endl;
            return;
        }

        fout << "source " << source << '\n';
        fout << "processor " << processor << '\n';
        fout << "align " << align << '\n';
        fout << "gaussianBlur " << gaussianBlur << '\n';
        fout << "maxDistance " << maxDistance << '\n';
        fout << "minDistance " << minDistance << '\n';
        fout << "drawDepth " << drawDepth << '\n';
        fout << "drawColor " << drawColor << '\n';
        fout << "fpsSetting " << fpsSetting << '\n';
        fout << "octaves " << octaves << '\n';
        fout << "seed " << seed << '\n';
        fout << "seedSize " << seedSize << '\n';
        fout << "persistance " << persistance << '\n';
        fout << "drawGrid " << drawGrid << '\n';
        fout << "temporalAlpha " << temporalAlpha << '\n';
        fout << "temporalDelta " << temporalDelta << '\n';
        fout << "temporalPersistance " << temporalPersistance << '\n';
        fout << "holeFill " << holeFill << '\n';

        fout << "warpPoints ";
        for (auto p : warpPoints)
        {
            fout << p.x << " " << p.y << " ";
        }
        fout << '\n';

        fout << "applyHeightAdjustment " << applyHeightAdjustment << '\n';

        fout << "planarPoints ";
        for (auto p : planarPoints)
        {
            fout << p.x << " " << p.y << " ";
        }
        fout << '\n';

        fout << "projectionPoints ";
        for (auto p : projectionPoints)
        {
            fout << p.x << " " << p.y << " ";
        }
        fout << '\n';

        fout << "drawLines " << drawLines << '\n';
        fout << "selectedShaderIndex " << selectedShaderIndex << '\n';
        fout << "drawContours " << drawContours << '\n';
        fout << "numberOfContourLines " << numberOfContourLines << '\n';
        fout << "drawProjection " << drawProjection << '\n';
    }

    void loadFromFile(const std::string & filename)
    {
        std::ifstream fin(filename);

        if (!fin.good())
        {
            std::cout << "Failed to open file: " << filename << std::endl;
            return;
        }

        std::string temp;
        while (fin >> temp)
        {
            if (temp == "source") { fin >> source; }
            if (temp == "processor") { fin >> processor; }
            if (temp == "align") { fin >> align; }
            if (temp == "gaussianBlur") { fin >> gaussianBlur; }
            if (temp == "maxDistance") { fin >> maxDistance; }
            if (temp == "minDistance") { fin >> minDistance; }
            if (temp == "drawDepth") { fin >> drawDepth; }
            if (temp == "drawColor") { fin >> drawColor; }
            if (temp == "fpsSetting") { fin >> fpsSetting; }
            if (temp == "octaves") { fin >> octaves; }
            if (temp == "seed") { fin >> seed; }
            if (temp == "seedSize") { fin >> seedSize; }
            if (temp == "persistance") { fin >> persistance; }
            if (temp == "drawGrid") { fin >> drawGrid; }
            if (temp == "temporalAlpha") { fin >> temporalAlpha; }
            if (temp == "temporalDelta") { fin >> temporalDelta; }
            if (temp == "temporalPersistance") { fin >> temporalPersistance; }
            if (temp == "holeFill") { fin >> holeFill; }
            if (temp == "warpPoints")
            {
                float x, y;
                for (auto & p : warpPoints)
                {
                    fin >> x >> y;
                    p = { x,y };
                }
            }
            if (temp == "planarPoints")
            {
                float x, y;
                for (auto & p : planarPoints)
                {
                    fin >> x >> y;
                    p = { x,y };
                }
            }
            if (temp == "projectionPoints")
            {
                float x, y;
                for (auto & p : projectionPoints)
                {
                    fin >> x >> y;
                    p = { x,y };
                }
            }
            if (temp == "drawLines") { fin >> drawLines; }
            if (temp == "selectedShaderIndex") { fin >> selectedShaderIndex; }
            if (temp == "drawContours") { fin >> drawContours; }
            if (temp == "numberOfContourLines") { fin >> numberOfContourLines; }
            if (temp == "drawProjection") { fin >> drawProjection; }
        }
    }
};