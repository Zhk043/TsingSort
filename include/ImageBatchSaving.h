#pragma once
#include <stdint.h>
#include <vector>
#include <sys/time.h>
#include <string>
#include <opencv2/opencv.hpp>
#include "Knight.hpp"

class ImageBatchSaving
{
private:
    const uint32_t batchImgLimit = 8000;
    uint32_t savedImgCount = 0;
    bool saveImgFlag = false;
    bool saveSymbolImgFlag = true;
    std::string GetCurrentTimeString();

public:
    bool GetFlag() { return saveImgFlag; }
    bool OverLimit() { return savedImgCount >= batchImgLimit; }
    void SetFlag(uint32_t flag, uint32_t mode);
    void SaveImg(cv::Mat &img, const DETECT_RESULT_S &result, const std::vector<std::string> &categoryNames);
};
