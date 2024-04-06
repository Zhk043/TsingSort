#pragma once
#include "DefineUtils.h"
#include <memory>
#include "Connect.h"
#include <vector>
#include <map>
#include "EventManager.h"

using namespace std;
class ScreenConnect
    : public EventCallback
{
public:
    ScreenConnect(int devId, int currFpgaIp);
    void StartConnect();
    virtual void Execute(std::shared_ptr<Event> event) override; // 回调重载函数

private:
    void HandleCmd(u8 *data, u16 data_len);
    void OpenCamera();
    void StopCamera();
    int ReceiveCommand(unsigned char *buf, int size);
    int SendCommand(unsigned char *buf, int len);
    int GetCPUTemp();
    int GetFileNum(char *folderPath);
    vector<string> GetFileNames(char *folderPath, int pageSize, int pageIndex);
    string GetImageNamesInfo(char *folderPath, int pageSize, int pageIndex);
    string GetFragmentInfo(char *fileName, int size, const char *data);
    void Export(string fileName, u8 boardId);
    string GetFileMd5(string fileName);

    int devId = 1;
    bool started = false;

    int screenSocket;
    sockaddr_in screenAddr;
    int currFpgaIp;
    int fragmentId = 0;
    int pkgFd = -1;
};

class ExportEvent : public Event
{
public:
    ExportEvent() : Event(EventType::Export) {}
    ExportEvent(std::string fileName, int boardId)
        : Event(EventType::Export), fileName_(fileName), boardId_(boardId) {}
    std::string fileName_;
    int boardId_;
};