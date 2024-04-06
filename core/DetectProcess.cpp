#include "DetectProcess.h"
#include "SynchProcess.h"
#include "FPGAProcess.h"
#include <unistd.h>
#include "LogManager.h"
#include "MemoryPool.h"

using namespace std;

extern std::shared_ptr<SynchProcess> synchProcess;
extern std::shared_ptr<FPGAProcess> fpgaProcess;
extern std::shared_ptr<PassProcess> passProcess;
extern std::shared_ptr<MemoryPool> memoryPool;
extern std::shared_ptr<KNIGHT> knight;

enum ProcessType
{
    Closed = 0,
    Normal,
    SaveImg,
    SaveContinusImg,
    TestValve,
    ChangeModel
};

DetectProcess::DetectProcess()
{
    isValveEnable = false; // 默认不开启吹阀
    processType = ProcessType::Closed;

    auto config = Config::GetInstance();
    models = config->models;
    modelId = models.begin()->first;
    lastModelId = modelId;
    isChangeModel = false;
    valveNum = config->valveNum;
    valveLeftIgnore = config->valveLeftIgnore;
    valveInterval = config->valveInterval;
    for (int i = 0; i < valveNum; ++i)
    {
        int left = valveLeftIgnore + valveInterval * i;
        int right = valveLeftIgnore + valveInterval * (i + 1) - 1;
        valvePixelPositions.emplace_back(left, right);
    }
    InitVariables();
}

void DetectProcess::InitVariables()
{
    auto config = Config::GetInstance();
    currModel = models[modelId];
    imgRows = currModel.imgRows;
    imgCols = currModel.imgCols;
    overlayLineNum = currModel.overlayLineNum;
}

DetectProcess::~DetectProcess()
{
}

void DetectProcess::DetectImage()
{
    std::cout << "current Model: " << currModel.path << std::endl;
    CLogInfo("current Model: " + currModel.path + ", currModel.category.num: " + to_string(currModel.category.num));
    std::string cfg = currModel.path + ".cfg";
    std::string weight = currModel.path + ".weight";
    knight->bAiInit(cfg.c_str(), weight.c_str(), currModel.category.num);
    while (true)
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        float recvImgInterval = 0.0;
        cv::Mat src = synchProcess->GetImageData(startTime, recvImgInterval);
#ifdef OPEN_LOG
        auto copyTime = std::chrono::high_resolution_clock::now();
        float copyInterval = std::chrono::duration_cast<std::chrono::microseconds>(copyTime - startTime).count() / 1000.0;
#endif
        switch (processType.load(std::memory_order_acquire))
        {
        case ProcessType::Normal:
        {
            DETECT_RESULT_S g_result;
#ifdef OPEN_LOG
            auto startDetectTime = std::chrono::high_resolution_clock::now();
            float detectInterval = 0.0;
#endif
            knight->bAiInference(src, &g_result);
#ifdef OPEN_LOG
            auto endDetectTime = std::chrono::high_resolution_clock::now();
            detectInterval = std::chrono::duration_cast<std::chrono::microseconds>(endDetectTime - startDetectTime).count() / 1000.0;
#endif
            if (g_result.u32BoxNum == 0)
                continue;
            DealDetectResult(src, g_result);
#ifdef OPEN_LOG
            auto endTime = std::chrono::high_resolution_clock::now();
            float interval = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count() / 1000.0;
            float dealInterval = std::chrono::duration_cast<std::chrono::microseconds>(endTime - endDetectTime).count() / 1000.0;
            std::string logStr = "all(ms): " + to_string(interval) + " detect(ms): " + to_string(detectInterval) + " recv(ms): " + to_string(recvImgInterval) + " copy(ms): " + to_string(copyInterval) + " deal(ms): " + to_string(dealInterval);
            count++;
            // if (count % 1000 == 0)
            CLogInfo(logStr);
            
#endif
        }
        break;
        case ProcessType::SaveImg:
        {
            DETECT_RESULT_S g_result;
            knight->bAiInference(src, &g_result);
            if (g_result.u32BoxNum == 0)
                continue;
            if (imgBatchSaving.OverLimit())
                continue;
            DealDetectResult(src, g_result);
            imgBatchSaving.SaveImg(src, g_result, currModel.category.names);
        }
        break;
        case ProcessType::SaveContinusImg:
        {
            if (imgContiSaving.GetType() == 0)
            {
                imgContiSaving.SaveImg(src);
            }
            else
            {
                DETECT_RESULT_S g_result;
                knight->bAiInference(src, &g_result);
                if (g_result.u32BoxNum > 0)
                    imgContiSaving.SaveImg(src);
            }
        }
        break;
        case ProcessType::TestValve:
            ProcessValveTest(src);
            break;
        case ProcessType::ChangeModel:
            isReadyChangeModel.store(true, std::memory_order_release);
            break;
        default:
            break;
        }
    }
}

void DetectProcess::DealDetectResult(cv::Mat &src, DETECT_RESULT_S &result)
{
    std::vector<BOX_S> blowObjects; // 所有待吹目标
    for (size_t i = 0; i < result.u32BoxNum; ++i)
    {
        passProcess->DealObjRectByPercent(result.stBox[i]);
        passProcess->DealDetectObj(src, result.stBox[i], blowObjects, currModel.category.num);
    }
    // 往喷阀发数据
    if (blowObjects.size() > 0 && isValveEnable)
    {
        CreateValveData(blowObjects, src, 0);
    }
}

void DetectProcess::CreateValveData(const std::vector<BOX_S> &objs, const cv::Mat &src, u8 isTest)
{
    memoryPool->ClearValveBuf();
    for (BOX_S obj : objs)
    {
        for (int line = obj.f32Ymin; line <= obj.f32Ymax; ++line)
        {
            for (int i = 0; i < valveNum; ++i)
            {
                if (valvePixelPositions[i].second >= obj.f32Xmin &&
                    valvePixelPositions[i].first <= obj.f32Xmax)
                {
                    memoryPool->SetValveData(line, i);
                }
            }
        }
    }

    // for (int line = 0; line < imgRows; ++line)
    // {
    //     for (int i = 0; i < valveNum; ++i)
    //     {
    //         int valvePixelPositionLeft = valveLeftIgnore + valveInterval * i;
    //         int valvePixelPositionRight = valveLeftIgnore + valveInterval * (i + 1) - 1;
    //         for (BOX_S obj : objs)
    //         {
    //             if (line >= obj.f32Ymin && line <= obj.f32Ymax && valvePixelPositionRight >= obj.f32Xmin && valvePixelPositionLeft <= obj.f32Xmax)
    //             {
    //                 memoryPool->SetValveData(line, i);
    //             }
    //         }
    //     }
    // }

    u16 rowId, overlayRowId;
    GetRowId(src, rowId, overlayRowId);
    u8 *buf = memoryPool->GetValveDataByBatch(0);
    fpgaProcess->SendValveData(buf, rowId, overlayRowId, isTest);
}

inline void DetectProcess::GetRowId(const cv::Mat &src, u16 &rowId, u16 &overlayRowId)
{
    cv::Vec3b firstPixel = src.at<cv::Vec3b>(0, 0);
    cv::Vec3b secondPixel = src.at<cv::Vec3b>(0, 1);
    u16 rowIdTmp = secondPixel[0];
    rowIdTmp <<= 8;
    rowIdTmp |= firstPixel[0];

    u16 overlayRowIdTmp;
    if (overlayLineNum > 0)
    {
        int overlayLineFirstRow = imgRows - overlayLineNum;
        firstPixel = src.at<cv::Vec3b>(overlayLineFirstRow, 0);
        secondPixel = src.at<cv::Vec3b>(overlayLineFirstRow, 1);
        overlayRowIdTmp = secondPixel[0];
        overlayRowIdTmp <<= 8;
        overlayRowIdTmp |= firstPixel[0];
    }
    else
    {
        overlayRowIdTmp = 0;
    }
    rowId = rowIdTmp;
    overlayRowId = overlayRowIdTmp;
}

void DetectProcess::RefreshProcessType()
{
    // 越靠上，优先级越高
    if (isChangeModel)
    {
        processType.store(ProcessType::ChangeModel, std::memory_order_release);
    }
    else if (valveTest.GetFlag())
    {
        processType.store(ProcessType::TestValve, std::memory_order_release);
    }
    else if (imgContiSaving.GetFlag())
    {
        processType.store(ProcessType::SaveContinusImg, std::memory_order_release);
    }
    else if (imgBatchSaving.GetFlag())
    {
        processType.store(ProcessType::SaveImg, std::memory_order_release);
    }
    else if (isValveEnable)
    {
        processType.store(ProcessType::Normal, std::memory_order_release);
    }
    else
    {
        processType.store(ProcessType::Closed, std::memory_order_release);
    }
}

void DetectProcess::ModifyModelId(char *value)
{
    modelId = value;
    if (modelId != lastModelId)
    {
        isReadyChangeModel.store(false, std::memory_order_release);
        isChangeModel = true;
        RefreshProcessType();
        bool changeFlag = false;
        while (true)
        {
            if (isReadyChangeModel.load(std::memory_order_acquire) || changeFlag)
            {
                InitVariables();
                RefreshModel();
                // 重新申请内存
                memoryPool->ReInit(imgRows, imgCols, overlayLineNum);

                isChangeModel = false;
                RefreshProcessType();
                break;
            }
            if (!changeFlag)
            {
                sleep(2);
                changeFlag = true;
            }
        }
    }
}

void DetectProcess::RefreshModel()
{
    if (modelId == lastModelId)
        return;
    std::cout << "current Model: " << currModel.path << std::endl;
    std::string cfg = currModel.path + ".cfg";
    std::string weight = currModel.path + ".weight";
    knight->bAiInit(cfg.c_str(), weight.c_str(), currModel.category.num);
    lastModelId = modelId;
}

// 批量采图模式
void DetectProcess::SetSaveImgFlag(u32 flag, u32 symbolFlag)
{
    imgBatchSaving.SetFlag(flag, symbolFlag);
    RefreshProcessType();
}
// 正常选料模式
void DetectProcess::SetValveEnable(u32 value)
{
    isValveEnable = value > 0;
    RefreshProcessType();
}
// 阀数量
void DetectProcess::SetValveNum(u32 value)
{
    int valveIntervalTmp = currModel.imgCols / valveNum;
    if (valveIntervalTmp == valveInterval)
    {
        valveNum = value;
        valveInterval = currModel.imgCols / value;
        valveLeftIgnore = (currModel.imgCols % value) / 2;
        valvePixelPositions.clear();
        for (int i = 0; i < valveNum; ++i)
        {
            int left = valveLeftIgnore + valveInterval * i;
            int right = valveLeftIgnore + valveInterval * (i + 1) - 1;
            valvePixelPositions.emplace_back(left, right);
        }
    }
}
// 连续采图模式
void DetectProcess::StartSaveContinuousImg(u32 type)
{
    imgContiSaving.Start(type, currModel.imgRows, currModel.imgCols, currModel.overlayLineNum);
    RefreshProcessType();
}
bool DetectProcess::GetContinuousImg(u32 rowId, u8 *buf)
{
    return imgContiSaving.GetImg(rowId, buf);
}

void DetectProcess::StopSaveContinuousImg()
{
    imgContiSaving.Stop();
    RefreshProcessType();
}
// 喷阀测试
void DetectProcess::StartTestValve()
{
    valveTest.Clear();
    valveTest.Start(valveNum, valveLeftIgnore, valveInterval);
    RefreshProcessType();
}

void DetectProcess::TestValve(u8 rate, u16 id)
{
    valveTest.Push(rate, id);
}

void DetectProcess::StopTestValve()
{
    valveTest.Stop();
    RefreshProcessType();
}

void DetectProcess::ProcessValveTest(const cv::Mat &src)
{
    if (valveTest.IsStop())
    {
        return;
    }
    std::vector<BOX_S> blowObjects = valveTest.ProcessValveTest();
    if (blowObjects.size() > 0)
    {
        CreateValveData(blowObjects, src, 1);
    }
}
