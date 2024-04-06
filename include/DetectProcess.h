#pragma once
#include <opencv2/opencv.hpp>
#include <atomic>
#include <vector>
#include <string>
#include "Config.h"
#include "PassProcess.h"
// #include "ThreadSafeQueue.h"
#include "ImageBatchSaving.h"
#include "ImageContinuousSaving.h"
#include "ValveTest.h"

class DetectProcess
{
public:
    DetectProcess();
    ~DetectProcess();
    void DetectImage();
    bool GetContinuousImg(u32 lineId, u8 *buf);
    void SetSaveImgFlag(u32 flag, u32 symbolFlag);
    void SetValveEnable(u32 value);
    void SetValveNum(u32 value);
    void StartSaveContinuousImg(u32 type);
    void StopSaveContinuousImg();
    void TestValve(u8 rate, u16 id);
    void ModifyModelId(char *value);
    void StartTestValve();
    void StopTestValve();

private:
    void InitVariables();
    void DealDetectResult(cv::Mat &src, DETECT_RESULT_S &result);
    void CreateValveData(const std::vector<BOX_S> &objs, const cv::Mat &src, u8 isTest);
    void GetRowId(const cv::Mat &src, u16 &rowId, u16 &overlayRowId);
    void ProcessValveTest(const cv::Mat &src);
    void RefreshModel();
    void RefreshProcessType();

private:
    ImageBatchSaving imgBatchSaving;
    ImageContinuousSaving imgContiSaving;
    ValveTest valveTest;

    std::atomic<int> processType;
    int imgRows;
    int imgCols;
    int overlayLineNum;
    bool isValveEnable;
    int valveNum;
    int valveLeftIgnore;
    int valveInterval;
    std::vector<std::pair<int, int>> valvePixelPositions;

    std::string lastModelId;
    std::string modelId;
    bool isChangeModel;
    std::atomic<bool> isReadyChangeModel;
    std::map<std::string, Model> models;
    Model currModel;
    long count = 0;
};