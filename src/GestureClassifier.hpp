#pragma once

struct GestureData
{
    double areaCB = 0.0;
    double areaCH = 0.0;
    double perimeterCH = 0.0;
    double maxD = 0.0;
    double minD = 0.0;
    double averageD = 0.0; 
    double pointsCH = 0.0; 
    double averageA = 0.0;
    std::array<int, 10> sliceCounts = { 0,0,0,0,0, 0,0,0,0,0 }; //9-18
    int classLabel = 0; 
};

class GestureClassifier
{
    double x_1, x_2, x_3, x_4, x_5, x_6, x_7, x_8, x_9, x_10, x_11, x_12, x_13, x_14, x_15, x_16, x_17, x_18;

    void parse(const GestureData & d)
    {
        x_1 = d.areaCB;
        x_2 = d.areaCH;
        x_3 = d.perimeterCH;
        x_4 = d.maxD;
        x_5 = d.minD;
        x_6 = d.averageD;
        x_7 = d.pointsCH;
        x_8 = d.averageA;
        x_9 = (double)d.sliceCounts[0];
        x_10 = (double)d.sliceCounts[1];
        x_11 = (double)d.sliceCounts[2];
        x_12 = (double)d.sliceCounts[3];
        x_13 = (double)d.sliceCounts[4];
        x_14 = (double)d.sliceCounts[5];
        x_15 = (double)d.sliceCounts[6];
        x_16 = (double)d.sliceCounts[7];
        x_17 = (double)d.sliceCounts[8];
        x_18 = (double)d.sliceCounts[9];
    }

    double class0() const
    {
        return -0.04 * x_11 - 0.14 * x_12 - 0.06 * x_13 - 0.07 * x_9 + 15.11 + 52.13 * exp(-0.45 * x_9);
    }

    double class1() const
    {
        return 2.27 * x_10 + 0.55 * x_14 + 2.59 * x_16 + 1.22 * x_18 - 1.38 * x_4 + 5.93 * x_5 - 422.78;
    }

    double class2() const
    {
        return -0.04 * x_10 - 0.e-2 * x_14 - 0.04 * x_16 - 0.02 * x_18 + 0.02 * x_4 - 0.1 * x_5 + 9.6 * exp(-0.06 * x_13 + 0.01 * x_9) - 4.03 + 19.14 * exp(-0.28 * x_9);
    }

    double class3() const
    {
        return 1.17 * x_10 + 0.28 * x_14 + 1.33 * x_16 + 0.63 * x_18 - 0.71 * x_4 + 3.06 * x_5 + 0.02 * pow(0.06 * x_10 + 0.04 * x_12 - 0.e-2 * x_14 - 0.32 * x_5 - 1, 2.0) - 215.59;
    }

    double class4() const
    {
        return 0.08 * pow(0.19 * x_10 + 0.13 * x_12 - 0.02 * x_14 - x_5 - 0.52, 2.0) - 17.12;
    }

    double class5() const
    {
        return 13.27 * sin(0.01 * x_12 + 9.16) + 2.71 * sin(0.03 * x_10 + 0.02 * x_12 - 0.15 * x_5 - 1.24) + 5.12;
    }

public:
    void classify(GestureData & data)
    {
        parse(data);
        std::array<double, 6> v = {class0(), class1(), class2(), class3(), class4(), class5()};
        auto iter = std::max_element(v.begin(), v.end());
        data.classLabel = (int)std::distance(v.begin(), iter);
    }
    void classify(std::vector<GestureData> & dataset)
    {
        for (auto & d : dataset)
        {
            classify(d);
        }
    }
};