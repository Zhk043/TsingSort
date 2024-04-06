#pragma once

#include <mutex>
#include <condition_variable>
#include <stdarg.h>
#include <memory>
#include <vector>
#include "ThreadSafeQueue.h"
#include "Connect.h"

enum class EventType
{
    Export,
};

// 事件基类 如有需要，自行支持移动操作
class Event
{
public:
    Event(EventType type_) : type(type_) {}
    EventType GetType() { return type; }
    virtual ~Event() {}

private:
    EventType type;
};

// class ExportEvent : public Event
// {
// public:
//     ExportEvent() : Event(EventType::Export) {}
//     ExportEvent(std::string fileName, int boardId, int sockFd, sockaddr_in addr)
//         : Event(EventType::Export), fileName_(fileName), boardId_(boardId), sockFd_(sockFd), addr_(addr) {}

//     std::string fileName_;
//     int boardId_;
//     int sockFd_;
//     sockaddr_in addr_;
// };

// 如有需要，自行支持移动操作
class EventCallback
{
public:
    EventCallback(EventType type_); // 一定要调用这个构造函数，不然id会是错误的
    virtual void Execute(std::shared_ptr<Event> event) = 0;
    EventType GetType() { return type; }
    int GetId() { return id; }

private:
    static int idTmp;
    int id; // 唯一id，不然会有问题
    EventType type;
    int priority; // todo
};

// class ExportCallback : public EventCallback
// {
// public:
//     ExportCallback() : EventCallback(EventType::Export) {}
//     virtual void Execute(std::shared_ptr<Event> event) override;
// };

// 适用于小量的事件管理类，当事件数量增加时，要添加红黑树和map来增加速度
class EventManager
{
public:
    EventManager();
    static EventManager *GetInstance();
    static void CreateInstance();

    void TriggerEvent(std::shared_ptr<Event> event);
    void RegisterEventCallback(std::shared_ptr<EventCallback> callback);
    void UnloadEventCallback(int callbackId);

    void Work();

private:
    static EventManager *instance;
    static std::once_flag m_flag;

    std::mutex eventMutex;
    std::condition_variable eventCond;

    std::vector<std::shared_ptr<EventCallback>> eventCallbacks;
    ThreadSafeQueue<std::shared_ptr<Event>> eventList;
    ThreadSafeQueue<std::shared_ptr<EventCallback>> registerEventCallbacks;
    ThreadSafeQueue<int> unloadEventCallbackIds;
};