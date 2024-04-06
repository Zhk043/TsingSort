#pragma once
#include <stdint.h>
#include <opencv2/opencv.hpp>

class ImageContinuousSaving
{
private:
    const uint32_t ContiImgRow = 1024;
    uint32_t type = 0; // 0:不需要物料，1：需要物料
    bool flag = false;
    uint32_t completedRowId;
    bool isSkipCurrImg = false;

    int imgRows;
    int imgCols;
    int overlayLineNum;
    cv::Mat continuousImg; // 保存1024行

public:
    uint32_t GetType() { return type; }
    bool GetFlag() { return flag; }
    void Start(uint32_t type_, int imgRows_, int imgCols_, int overlayLineNum_);
    void SaveImg(const cv::Mat &img);
    bool GetImg(uint32_t requestRowId, uint8_t *buf);
    void Stop() { flag = false; }
};
