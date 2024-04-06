#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include "DefineUtils.h"
#include "Knight.hpp"

class PassProcess
{
public:
    PassProcess();

    void SetGlobalImgCut(u32 dir, u32 value);          // 0、1、2、3对应上下左右，value以100为分界，高于50为正，低于50为负
    void SetGlobalImgCutByPercent(u32 dir, u32 value); // 0、1、2、3对应上下左右，value以100为分界，大于100为缩小，小于100为放大
    void SetHalfImgCut(u32 dir, u32 value);
    void SetEnable(u32 value);
    void SetConf(u32 conf, int id);
    void SetConf(std::vector<int> confs);
    void SetMatterRatio(u32 value);
    void SetFilterArea(u32 value);
    void SetLabelId(u32 value);
    void SetCutoff(u32 value);
    void SetLowerLimitHeight(u32 value);
    void DealObjRectByPercent(BOX_S &obj);
    void DealDetectObj(cv::Mat &src, const BOX_S &obj, std::vector<BOX_S> &objs, int labelNum);

private:
    std::vector<float> labelConfs;
    cv::Rect globalImgCut;
    cv::Rect halfImgCut;
    int globalImgCutTmp[4];
    int halfImgCutTmp[4];
    float matterRatio;
    int filterArea;
    int labelIndex;
    int cutoff;
    int lowerLimitHeight;
    bool isEnable;
};