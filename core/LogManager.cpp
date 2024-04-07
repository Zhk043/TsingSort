//日志管理
#include "LogManager.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;

LogManager *LogManager::instance = nullptr;
std::once_flag LogManager::m_flag;

LogManager *LogManager::GetInstance()
{
    if (instance == nullptr)
    {
        std::call_once(m_flag, CreateInstance);
    }
    return instance;
}

void LogManager::CreateInstance()
{
    instance = new (std::nothrow) LogManager();
    if (instance == nullptr)
    {
        throw std::exception();
    }
}

LogManager::LogManager()
{
    time_t t;
    int year, month, day;
    tm *lt;

    t = time(NULL);
    lt = localtime(&t);

    year = lt->tm_year + 1900;
    month = lt->tm_mon + 1;
    day = lt->tm_mday;

    std::string timeDay = to_string(year) + "_" + to_string(month) + "_" + to_string(day);
    fileName = "./log/" + timeDay + ".txt";

    checkLogSizeSymbol = 0;
}

void LogManager::Info(std::string info)
{
    LogObject log(LogLevel::INFO, info, std::time(nullptr));
    Log(log);
    // std::cout << "[info]" << info << std::endl;
}

void LogManager::Error(std::string error)
{
    LogObject log(LogLevel::ERROR, error, std::time(nullptr));
    Log(log);
    // std::cout << "[error]" << error << std::endl;
}

void LogManager::Log(const LogObject &log)
{
    if (checkLogSizeSymbol == 0 || checkLogSizeSymbol > 10000) // 开机的时候检查一次和每N次检查一次
    {
        struct stat statbuf;
        if (stat(fileName.c_str(), &statbuf) == 0)
        {
            if (statbuf.st_size > 64 * 1024 * 1024)
            { // 日志已经打印了64Mb，就删掉日志
                remove(fileName.c_str());
            }
        }
        checkLogSizeSymbol = 0;
    }
    ++checkLogSizeSymbol;
    std::fstream file;
    file.open(fileName.c_str(), std::ios::out | std::ios::app);
    std::tm logTime = *std::localtime(&log.time);
    file << "[" << logTime.tm_hour << ":" << logTime.tm_min << ":" << logTime.tm_sec << "]";
    switch (log.logLevel)
    {
    case LogLevel::ERROR:
        file << "[error]";
        break;
    default:
        file << "[info]";
        break;
    }
    file << log.log << std::endl;
}
