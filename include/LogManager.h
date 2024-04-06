#pragma once
#include <thread>
#include <string>
#include "ThreadSafeQueue.h"
#include <mutex>
#include <condition_variable>
#include <ctime>
#include <vector>

enum class LogLevel
{
    INFO,
    ERROR
};

class LogObject
{
public:
    LogLevel logLevel;
    std::string log;
    std::time_t time;
    LogObject() {}
    LogObject(LogLevel level, std::string &str, std::time_t t) : logLevel(level), log(str), time(t) {}
    LogObject(LogObject &&other)
    {
        logLevel = other.logLevel;
        log = std::move(other.log);
        time = other.time;
    }
    LogObject &operator=(LogObject &&other)
    {
        logLevel = other.logLevel;
        log = std::move(other.log);
        time = other.time;
        return *this;
    }
};

class LogManager
{
public:
    static LogManager *GetInstance();
    static void CreateInstance();
    LogManager();
    void Info(std::string info);
    void Error(std::string error);

private:
    void Log(const LogObject &log);

private:
    ThreadSafeQueue<LogObject> logQueue;

    std::mutex logMutex;
    std::condition_variable logCond;
    std::string fileName;
    unsigned checkLogSizeSymbol;

    static LogManager *instance;
    static std::once_flag m_flag;
};

#ifdef OPEN_LOG //想要开启log，在CMake里面添加OPEN_LOG宏
#define CLogInfo(x) LogManager::GetInstance()->Info(x)
#define CLogError(x) LogManager::GetInstance()->Error(x)
#else
#define CLogInfo(x)
#define CLogError(x)
#endif
