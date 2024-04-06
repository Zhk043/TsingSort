#pragma once
#include <opencv2/opencv.hpp>
#include <mutex>
#include <condition_variable>
#include "DefineUtils.h"
#include <chrono>

// class ImgData
// {
// public:
//     cv::Mat src;
//     std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
//     int recvImgInterval;

//     ImgData() {}
//     ImgData(cv::Mat &src_, std::chrono::time_point<std::chrono::high_resolution_clock> startTime_, int recvImgInterval_)
//         : src(src_), startTime(startTime_), recvImgInterval(recvImgInterval_)
//     {
//     }
// };

class SynchProcess
{
public:
    SynchProcess();
    ~SynchProcess();
    void SynchReceiveImage(int fpgaImgDataIndex);
    void SynchTime(int fpgaImgDataIndex,std::chrono::time_point<std::chrono::high_resolution_clock> startTime);
    cv::Mat GetImageData(std::chrono::time_point<std::chrono::high_resolution_clock> &startTime, float &recvImgInterval);
    void GetSomeImageData(u8 *buf, int lineId);

private:
    int detectImgDataIndex;
    int imgDataIndex;
    std::mutex dataMutex;
    std::condition_variable dataCond;
    std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>> startTimes;
    std::vector<float> recvImgIntervals;
};