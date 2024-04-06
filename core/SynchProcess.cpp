#include "SynchProcess.h"
#include "MemoryPool.h"

extern std::shared_ptr<MemoryPool> memoryPool;

SynchProcess::SynchProcess()
{
    imgDataIndex = 0;
    detectImgDataIndex = 0;
    startTimes.resize(CacheImgSize);
    recvImgIntervals.resize(CacheImgSize);
}

SynchProcess::~SynchProcess()
{
}

void SynchProcess::SynchReceiveImage(int fpgaImgDataIndex)
{
    imgDataIndex = fpgaImgDataIndex;
    dataCond.notify_one();
}

void SynchProcess::SynchTime(int fpgaImgDataIndex, std::chrono::time_point<std::chrono::high_resolution_clock> startTime)
{
    startTimes[fpgaImgDataIndex] = startTime;
    auto currTime = std::chrono::high_resolution_clock::now();
    recvImgIntervals[fpgaImgDataIndex] = std::chrono::duration_cast<std::chrono::microseconds>(currTime - startTime).count() / 1000.0;
}

cv::Mat SynchProcess::GetImageData(std::chrono::time_point<std::chrono::high_resolution_clock> &startTime, float &recvImgInterval)
{
    int detectImgDataIndexTmp;
    {
        std::unique_lock<std::mutex> dataLock(dataMutex);
        dataCond.wait(dataLock, [&]
                      { return detectImgDataIndex != imgDataIndex; });
        detectImgDataIndexTmp = detectImgDataIndex;
        detectImgDataIndex = (detectImgDataIndex + 1) % CacheImgSize;
    }

    // cv::Mat img(memoryPool->GetImgRows(), memoryPool->GetImgCols(), CV_8UC3, memoryPool->GetImageByIndex(detectImgDataIndexTmp));
    cv::Mat img = cv::Mat(memoryPool->GetImgRows(), memoryPool->GetImgCols(), CV_8UC3);
    memcpy(img.data, memoryPool->GetImageByIndex(detectImgDataIndexTmp), memoryPool->GetImgRows() * memoryPool->GetImgCols() * 3);
#ifdef OPEN_LOG
    startTime = startTimes[detectImgDataIndexTmp];
    recvImgInterval = recvImgIntervals[detectImgDataIndexTmp];
#endif
    return img;
}

void SynchProcess::GetSomeImageData(u8 *buf, int lineId)
{
    u8 *dataBuf = memoryPool->GetReceiveImageBuf(0, memoryPool->GetImgRows() - lineId, 0);
    memcpy(buf, dataBuf, memoryPool->GetImgCols() * 3);
}
