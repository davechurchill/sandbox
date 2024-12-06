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

    double noGesture() const
    {
        return -0.15 * x_11 - 0.13 * x_13 - 0.07 * x_15 - 0.09 * x_16 + 0.25 * x_18 + 0.36 * x_4 - 0.42 * x_6 - 0.05 * x_9 - 39.29 * sin(2.58 * x_2 - 9.59) + 28.17 + 0.72 / pow(0.41 - x_3, 2);
    }

    double yesGesture() const
    {
        return -0.24 * x_11 + 0.18 * x_14 - 0.25 * x_4 + 0.62 * x_6 - 0.22 * x_9 + 9.59 * sqrt(0.05 * x_18 + 1) + 23.87 * sin(2.58 * x_2 - 9.59) + 2.21 * tanh(3.54 * x_3 - 3.32) - 55.87;
    }

    double class1() const
    {
        return -0.28 * x_10 - 1.0 * x_11 + 0.93 * x_12 + 2.65 * x_13 + 1.19 * x_14 + 0.6 * x_15 + 2.79 * x_16 + 0.36 * x_17 + 0.15 * x_18 + 0.88 * x_3 - 8.46 * x_4 - 4.24 * x_5 + 8.36 * x_6 + 15.96 * cos(0.02 * x_5 + 4.78) + 2.91 * cos(0.54 * x_8 - 4.81) + 122.17;
    }

    double class2() const
    {
        return -0.02 * x_10 - 0.63 * x_11 + 3.22 * x_12 + 1.31 * x_13 + 0.15 * x_14 + 3.02 * x_15 - 4.76 * x_16 - 2.41 * x_17 - 2.63 * x_18 + 9.09 * x_3 + 36.97 * x_4 - 15.47 * x_5 - 55.02 * x_6 - 6.01 * x_9 - 63.24 * cos(0.02 * x_5 + 4.78) - 19.6 * cos(0.54 * x_8 - 4.81) + 118.81;
    }

    double class3() const
    {
        return -0.22 * x_10 - 0.65 * x_11 - 0.38 * x_12 + 1.07 * x_13 + 0.55 * x_14 - 0.4 * x_15 + 2.53 * x_16 + 0.43 * x_17 + 0.7 * x_18 - 1.62 * x_3 - 13.2 * x_4 + 1.04 * x_5 + 18.05 * x_6 + 1.18 * x_9 + 23.51 * cos(0.02 * x_5 + 4.78) + 6.52 * cos(0.54 * x_8 - 4.81) + 59.0;
    }

    double class4() const
    {
        return 0.9 * x_10 + 3.64 * x_11 - 4.9 * x_12 - 8.28 * x_13 - 3.32 * x_14 - 3.99 * x_15 - 3.89 * x_16 + 1.37 * x_17 + 1.62 * x_18 - 9.58 * x_3 - 5.41 * x_4 + 24.24 * x_5 + 18.75 * x_6 + 5.02 * x_9 + 4.93 * cos(0.02 * x_5 + 4.78) + 6.68 * cos(0.54 * x_8 - 4.81) - 498.09;
    }

    double class5() const
    {
        return -0.47 * x_10 - 1.72 * x_11 + 1.18 * x_12 + 3.37 * x_13 + 1.43 * x_14 + 0.93 * x_15 + 3.11 * x_16 - 0.2 * x_17 + 0.04 * x_18 + 1.74 * x_3 - 8.41 * x_4 - 6.81 * x_5 + 8.22 * x_6 - 0.73 * x_9 + 16.49 * cos(0.02 * x_5 + 4.78) + 3.04 * cos(0.54 * x_8 - 4.81) + 39.3;
    }

public:
    void classify(GestureData & data)
    {
        parse(data);
        if (yesGesture() > noGesture())
        {
            std::array<double, 5> v = { class1(), class2(), class3(), class4(), class5() };
            auto iter = std::max_element(v.begin(), v.end());
            data.classLabel = (int)std::distance(v.begin(), iter) + 1;
        }
        else
        {
            data.classLabel = 0;
        }
    }
    void classify(std::vector<GestureData> & dataset)
    {
        for (auto & d : dataset)
        {
            classify(d);
        }
    }
};