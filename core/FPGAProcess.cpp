#include "FPGAProcess.h"
#include "Config.h"
#include "MemoryPool.h"
#include "Knight.hpp"
#include "LogManager.h"

using namespace std;

#define NET_HEADER 15
extern std::shared_ptr<MemoryPool> memoryPool;
extern std::shared_ptr<KNIGHT> knight;

FPGAProcess::FPGAProcess(int fpgaIp_)
    : fpgaIp(fpgaIp_)
{
    overlayValveRowId = 0x1 << 16; // 初始化一个大值
    preImgId = 0;
    recvRows = 0;
    imgDataIndex = 0;
    CreateSocket();
}

void FPGAProcess::CreateSocket()
{
    auto config = Config::GetInstance();

    if ((fpgaSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket() error");
        return;
    }

    sockaddr_in localAddr; // 往fpga通信的本地ip地址
    bzero(&localAddr, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &localAddr.sin_addr);
    localAddr.sin_port = htons(config->fpgaLocalPort);

    if (bind(fpgaSocket, (struct sockaddr *)(&localAddr), sizeof(localAddr)) < 0) // 绑定本地ip地址
    {
        perror("fpga local address bind() error");
        return;
    }

    // 发送地址
    bzero(&fpgaAddr, sizeof(fpgaAddr));
    fpgaAddr.sin_family = AF_INET;
    inet_pton(AF_INET, config->fpgaRemoteAddress.c_str(), &fpgaAddr.sin_addr);
    fpgaAddr.sin_port = htons(config->fgpaPort); // 直接2001端口发送数据
    // fpgaAddr.sin_port = htons(config->screenPort); // 因为只能通过2000端口发送数据，所以统一从转发程序转发
}

FPGAProcess::~FPGAProcess()
{
}

void FPGAProcess::ReceiveImage()
{
    // 绑定CPU核
    knight->bSetCpu(2, fpgaSocket);
    while (true)
    {
        ReceiveCamera();
    }
}

bool FPGAProcess::ReceiveCamera()
{
    int imgRows = memoryPool->GetImgRows();
    int imgCols = memoryPool->GetImgCols();
    int overlayLineNum = memoryPool->GetOverlayLineNum();
    int onePacketNRows = Config::GetInstance()->onePacketNRows;
    int recvRowTmp = recvRows + overlayLineNum;
    u8 *buf = memoryPool->GetReceiveImageBuf(0, recvRowTmp, imgDataIndex);
    u16 rowId;
    int ret = recvfrom(fpgaSocket, buf, imgCols * 3 * onePacketNRows, 0, nullptr, nullptr);
    if (ret > 0)
    {
        rowId = buf[3];
        rowId <<= 8;
        rowId |= buf[0];
        if ((buf[0] != buf[1]) || (rowId <= preImgId && preImgId - rowId < 10000))
            return false;
#ifdef OPEN_LOG
        u16 preTmp = preImgId + onePacketNRows;
        if (rowId != preTmp)
        {
            std::string lossRowStr = "preRowId: " + std::to_string(preImgId) + " ##currRowId: " + std::to_string(rowId);
            CLogInfo(lossRowStr);
        }
#endif
        preImgId = rowId;
    }
    else
    {
        perror("fpgaProcess receive error");
        return false;
    }
    if (recvRowTmp == overlayLineNum)
    {
        startTime = std::chrono::high_resolution_clock::now();
    }
    if (recvRowTmp >= imgRows - overlayLineNum)
    {
        int overlayLineIndex = recvRowTmp + overlayLineNum - imgRows;
        u8 *destBuf = memoryPool->GetOverlayImageBuf(0, overlayLineIndex, imgDataIndex);
        memcpy(destBuf, buf, imgCols * 3 * onePacketNRows);
    }

    recvRows = (recvRowTmp - overlayLineNum + onePacketNRows) % (imgRows - overlayLineNum);
    if (recvRows == 0)
    {
#ifdef OPEN_LOG
        synchProcess->SynchTime(imgDataIndex, startTime);
#endif
        imgDataIndex = (imgDataIndex + 1) % CacheImgSize;
        synchProcess->SynchReceiveImage(imgDataIndex);
    }
    return true;
}

const unsigned char command_pre[] = {0x00, 0x02, 0x00, 0x00, 0xb0, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void FPGAProcess::SendValveData(u8 *linescommand, u16 rowid, u16 overlayRowId, u8 isTest)
{
    int imgRows = memoryPool->GetImgRows();
    // int blowDataRowNum = Config::GetInstance()->valveBlowLineNum;
    int blowDataRowNum = min(imgRows, Config::GetInstance()->valveBlowLineNum);
    int totalValveNum = Config::GetInstance()->valveNum * Config::GetInstance()->oneValveNum;
    int valveNeedByteNum = max(8, int(ceil(double(totalValveNum) / 8.0))); // 与FPGA协议通信，阀数据按照至少8字节处理，实际少于8字节的部分数据补零

    int overlayLineNum = memoryPool->GetOverlayLineNum();
    if (overlayLineNum > 0)
    {
        if ((u32)rowid == overlayValveRowId)
        {
            memoryPool->GetOverlayValveData(linescommand, 0);
        }

        memoryPool->SetOverlayValveData(linescommand, 0);
        overlayValveRowId = overlayRowId;
    }

    const int len = blowDataRowNum * valveNeedByteNum + 4; // 128*8+4=1028
    u8 lenHigh = len / 256;
    u8 lenLow = len % 256;
    unsigned char commandChar[len + NET_HEADER] = {0}; // 1024 +15 +4

    for (int i = 0; i < NET_HEADER; ++i)
    {
        if (i == 0)
        {
            u8 addr = fpgaIp;
            commandChar[i] = addr;
        }
        else if (i == 6)
        {
            commandChar[i] = lenHigh;
        }
        else if (i == 7)
        {
            commandChar[i] = lenLow;
        }
        else
        {
            commandChar[i] = command_pre[i];
        }
    }

    int commandCount = ceil(double(imgRows) / blowDataRowNum); // 下发数据不支持巨帧，拆分成多个128行
    for (int i = 0; i < commandCount; i++)
    {
        unsigned char lineNumHigh = (rowid + i * blowDataRowNum) / 256;
        unsigned char lineNumLow = (rowid + i * blowDataRowNum) % 256;

        commandChar[NET_HEADER] = lineNumLow;
        commandChar[NET_HEADER + 1] = lineNumHigh;
        commandChar[NET_HEADER + 2] = isTest;

        if (i == commandCount - 1 && commandCount > 1)
        {
            int dataLen = (imgRows - i * blowDataRowNum) * valveNeedByteNum;
            memcpy(commandChar + NET_HEADER + 4, linescommand + i * blowDataRowNum * valveNeedByteNum, dataLen);
            memset(commandChar + NET_HEADER + 4 + dataLen, 0, blowDataRowNum * valveNeedByteNum - dataLen);
        }
        else
        {
            memcpy(commandChar + NET_HEADER + 4, linescommand + i * blowDataRowNum * valveNeedByteNum, blowDataRowNum * valveNeedByteNum);
        }

        // std::cout << "send data" << std::endl;
        // std::string data = "";
        // char buf[3] = {0};
        // for (int i = 0; i < 100; i++)
        // {
        //     sprintf(buf, "%02x", (int)commandChar[i]);
        //     data += buf;
        //     data += " ";
        // }
        // std::cout << data << std::endl;

        int n = sendto(fpgaSocket, commandChar, blowDataRowNum * valveNeedByteNum + NET_HEADER + 4, 0, (sockaddr *)(&fpgaAddr), sizeof(fpgaAddr));
        if (n < 0)
        {
            perror("valve data send error");
        }
    }
}
