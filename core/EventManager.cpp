//事件管理器
#include "EventManager.h"
#include <thread>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>

int EventCallback::idTmp = 0;

EventCallback::EventCallback(EventType type_)
    : id(idTmp++), type(type_)
{
}

// void ExportCallback::Execute(std::shared_ptr<Event> event)
// {
//     std::shared_ptr<ExportEvent> eventTmp = std::dynamic_pointer_cast<ExportEvent>(event);
//     std::string fileName = eventTmp->fileName_;
//     u8 boardId = eventTmp->boardId_;
//     int sockFd = eventTmp->sockFd_;
//     sockaddr_in addr = eventTmp->addr_;
    
//     YLS_ENET_FRAME packet;
//     memset(&packet, 0, sizeof(packet));
//     packet.boardId = boardId;
//     packet.operatorID = 0xB0;
//     packet.startAddress = htonl(0x00000165);
//     packet.packetId = 0;

//     std::ifstream in(fileName, std::ios::binary);
//     const int FRAG_SIZE = 8000; // 相机板单次传输上限8k(8096)
//     char dataBuf[FRAG_SIZE] = {0};
//     int fragId = 0;
//     int isLast = 0;
//     struct stat statBuf;
//     stat(fileName.c_str(), &statBuf);
//     int size = statBuf.st_size / FRAG_SIZE;
//     while (in.read(dataBuf, FRAG_SIZE))
//     {
//         int fragSize = in.gcount();
//         if (fragSize != FRAG_SIZE || fragId == size - 1)
//             isLast = 1;
        
//         memcpy(packet.Bbuffer, dataBuf, fragSize);
//         packet.len = htons(fragSize);
//         packet.rev1 = (u8)isLast;
//         packet.rev2 = htonl(fragId);
//         // SendCommand((u8 *)&packet, YLS_NET_HEADER + fragSize);
//         sendto(sockFd, (u8 *)&packet, YLS_NET_HEADER + fragSize, 0, (sockaddr *)(&addr), sizeof(addr));

//         fragId++;
//         std::cout << "fragId:" << fragId << std::endl;
//         usleep(1000);  //延时1ms，相机版缓冲有限
//     }
//     in.close();
// }

EventManager *EventManager::instance = nullptr;
std::once_flag EventManager::m_flag;

EventManager *EventManager::GetInstance()
{
    if (instance == nullptr)
    {
        std::call_once(m_flag, CreateInstance);
    }
    return instance;
}

void EventManager::CreateInstance()
{
    instance = new (std::nothrow) EventManager();
    if (instance == nullptr)
    {
        throw std::exception();
    }
}

EventManager::EventManager()
{
    std::thread th(&EventManager::Work, this);
    th.detach(); 
}

void EventManager::Work()
{
    while (true)
    {
        std::unique_lock<std::mutex> eventLock(eventMutex);
        eventCond.wait(eventLock, [&]
                       { return !eventList.empty() || !registerEventCallbacks.empty() || unloadEventCallbackIds.empty(); });

        std::vector<int> unloadEventCallbackIdList;
        std::vector<std::shared_ptr<EventCallback>> registerEventCallbackList;
        int eventCallbackId;
        while (unloadEventCallbackIds.try_pop(eventCallbackId))
        {
            unloadEventCallbackIdList.push_back(eventCallbackId);
        }

        std::shared_ptr<EventCallback> callback;
        while (registerEventCallbacks.try_pop(callback))
        {
            registerEventCallbackList.push_back(callback);
        }

        for (size_t i = 0; i < unloadEventCallbackIdList.size(); ++i)
        {
            int callbackId = unloadEventCallbackIdList[i];
            for (size_t j = 0; j < registerEventCallbackList.size(); ++j)
            {
                if (registerEventCallbackList[i]->GetId() == callbackId)
                {
                    registerEventCallbackList.erase(registerEventCallbackList.begin() + i);
                    break;
                }
            }
            for (size_t j = 0; j < eventCallbacks.size(); ++j)
            {
                if (eventCallbacks[i]->GetId() == callbackId)
                {
                    eventCallbacks.erase(eventCallbacks.begin() + i);
                    break;
                }
            }
        }

        for(size_t i = 0; i < registerEventCallbackList.size(); ++i){
            eventCallbacks.push_back(registerEventCallbackList[i]);
        }

        std::shared_ptr<Event> event;
        while (eventList.try_pop(event))
        {
            for (size_t i = 0; i < eventCallbacks.size(); ++i)
            {
                if (eventCallbacks[i]->GetType() == event->GetType())
                {
                    eventCallbacks[i]->Execute(event);
                }
            }
        }
    }
}

void EventManager::TriggerEvent(std::shared_ptr<Event> event)
{
    eventList.push(std::move(event));
    eventCond.notify_one();
}

void EventManager::RegisterEventCallback(std::shared_ptr<EventCallback> callback)
{
    registerEventCallbacks.push(std::move(callback));
    eventCond.notify_one();
}

void EventManager::UnloadEventCallback(int callbackId)
{
    unloadEventCallbackIds.push(std::move(callbackId));
    eventCond.notify_one();
}