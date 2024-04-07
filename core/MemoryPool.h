//内存池
#include "DefineUtils.h"
#include <math.h>
#include <memory>
#include <algorithm>

#define CacheImgSize 8
class MemoryPool
{
private:
    int imgRows_;
    int imgCols_;
    int batchNum_;
    int overlayLineNum_;
    int valveNum_;
    int oneValveNum_;
    int bgColorR_;
    int bgColorG_;
    int bgColorB_;
    int oneImgSize;
    u8 *imgDataBuf;
    u8 *oneLineBackground;

    int valveNeedByteNum;
    int valveDataBufSize;
    u8 *valveDataBuf;
    u8 *overlayValveData;

private:
    void Cleanup()
    {
        if (imgDataBuf)
        {
            delete[] imgDataBuf;
            imgDataBuf = nullptr;
        }
        if (oneLineBackground)
        {
            delete[] oneLineBackground;
            oneLineBackground = nullptr;
        }
        if (valveDataBuf)
        {
            delete[] valveDataBuf;
            valveDataBuf = nullptr;
        }
        if (overlayValveData)
        {
            delete[] overlayValveData;
            overlayValveData = nullptr;
        }
    }

    void Init(int imgRows, int imgCols, int batchNum, int valveNum, int oneValveNum, int overlayLineNum, int bgColorR, int bgColorG, int bgColorB)
    {
        imgRows_ = imgRows;
        imgCols_ = imgCols;
        batchNum_ = batchNum;
        valveNum_ = valveNum;
        oneValveNum_ = oneValveNum;
        overlayLineNum_ = overlayLineNum;
        bgColorR_ = bgColorR;
        bgColorG_ = bgColorG;
        bgColorB_ = bgColorB;
        // 图像数据
        oneImgSize = imgRows * imgCols * 3;
        imgDataBuf = new u8[oneImgSize * batchNum * CacheImgSize]();
        for (int i = 0; i < imgRows * imgCols * batchNum * CacheImgSize; ++i)
        {
            imgDataBuf[i * 3] = bgColorR;
            imgDataBuf[i * 3 + 1] = bgColorG;
            imgDataBuf[i * 3 + 2] = bgColorB;
        }
        oneLineBackground = new u8[3 * imgCols];
        for (int i = 0; i < imgCols; ++i)
        {
            oneLineBackground[i * 3] = bgColorR;
            oneLineBackground[i * 3 + 1] = bgColorG;
            oneLineBackground[i * 3 + 2] = bgColorB;
        }
        // 阀数据
        valveNeedByteNum = std::max(8, int(ceil(double(valveNum * oneValveNum) / 8.0))); // 与FPGA协议通信，阀数据按照至少8字节处理，实际少于8字节的部分数据补零
        valveDataBufSize = imgRows * batchNum * valveNeedByteNum;
        valveDataBuf = new u8[valveDataBufSize]();
        if (overlayLineNum > 0)
        {
            overlayValveData = new u8[valveNeedByteNum * batchNum * overlayLineNum]();
        }
        else
        {
            overlayValveData = nullptr;
        }
    }

public:
    MemoryPool(int imgRows, int imgCols, int batchNum, int valveNum, int oneValveNum, int overlayLineNum, int bgColorR, int bgColorG, int bgColorB)
    {
        imgDataBuf = nullptr;
        oneLineBackground = nullptr;
        valveDataBuf = nullptr;
        overlayValveData = nullptr;
        Init(imgRows, imgCols, batchNum, valveNum, oneValveNum, overlayLineNum, bgColorR, bgColorG, bgColorB);
    }

    void ReInit(int imgRows, int imgCols, int overlayLineNum)
    {
        Cleanup();
        Init(imgRows, imgCols, batchNum_, valveNum_, oneValveNum_, overlayLineNum, bgColorR_, bgColorG_, bgColorB_);
    }

    u8 *GetReceiveImageBuf(int batchId, int recvRows, int dataIndex)
    {
        u8 *buf = imgDataBuf + batchId * oneImgSize + dataIndex * oneImgSize * batchNum_ + recvRows * imgCols_ * 3;
        return buf;
    }

    u8 *GetOverlayImageBuf(int batchId, int recvRows, int dataIndex)
    {
        int imgDataIndexTmp = (dataIndex + 1) % CacheImgSize;
        u8 *buf = imgDataBuf + batchId * oneImgSize + imgDataIndexTmp * oneImgSize * batchNum_ + recvRows * imgCols_ * 3;
        return buf;
    }

    u8 *GetImageByIndex(int imgIndex)
    {
        u8 *buf = imgDataBuf + imgIndex * oneImgSize * batchNum_;
        return buf;
    }

    u8 *GetOneLineBackground() { return oneLineBackground; }

    void ClearValveBuf() { memset(valveDataBuf, 0, valveDataBufSize); }

    u8 *GetValveBuf() { return valveDataBuf; }

    void SetValveData(int rowIndex, int valveIndex)
    {
        u8 *dataPoint = valveDataBuf + valveNeedByteNum * rowIndex + valveIndex / 8;
        int valveByteShift = valveIndex % 8;
        *dataPoint = *dataPoint | 0x1 << valveByteShift;
    }

    u8 *GetValveDataByBatch(int batchId)
    {
        return valveDataBuf + batchId * imgRows_ * valveNeedByteNum;
    }

    void GetOverlayValveData(u8 *&batchValveData, int batchId)
    {
        for (int i = 0; i < overlayLineNum_; ++i)
        {
            for (int j = 0; j < valveNeedByteNum; ++j)
            {
                batchValveData[i * valveNeedByteNum + j] |= overlayValveData[batchId * overlayLineNum_ * valveNeedByteNum + i * valveNeedByteNum + j]; // 或关系，上一张图像检测到 或 本张图像检测到
            }
        }
    }

    void SetOverlayValveData(u8 *batchValveData, int batchId)
    {
        memcpy(overlayValveData + batchId * overlayLineNum_ * valveNeedByteNum, batchValveData + (imgRows_ - overlayLineNum_) * valveNeedByteNum, overlayLineNum_ * valveNeedByteNum);
    }
    int GetImgRows() { return imgRows_; }
    int GetImgCols() { return imgCols_; }
    int GetOverlayLineNum() { return overlayLineNum_; }
};
