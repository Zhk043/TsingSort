#pragma once
#include "Connect.h"
#include <opencv2/opencv.hpp>
#include <vector>
#include <chrono>
#include "SynchProcess.h"
#include <memory>
#include <atomic>

class FPGAProcess
{
public:
    FPGAProcess(int fpgaIp);
    ~FPGAProcess();

    void ReceiveImage();
    void SendValveData(u8 *linescommand, u16 rowid, u16 overlayRowId, u8 isTest);
    void SetSynchProcess(std::shared_ptr<SynchProcess> &process) { synchProcess = process; }

private:
    void CreateSocket();
    bool ReceiveCamera(); // 接受相机数据

private:
    u32 overlayValveRowId;
    int fpgaIp;
    int fpgaSocket;
    sockaddr_in fpgaAddr;
    u16 preImgId;
    std::shared_ptr<SynchProcess> synchProcess;
    int imgDataIndex;
    int recvRows;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
};