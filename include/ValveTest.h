#pragma once
#include <stdint.h>
#include <iostream>
#include "ThreadSafeQueue.h"
#include "Config.h"
#include "Knight.hpp"

struct ValveTestData
{
    ValveTestData() : rate(0), id(0), batchId(0) {}
    ValveTestData(uint8_t rate_, uint16_t id_, int batchId_) : rate(rate_), id(id_), batchId(batchId_) {}
    uint8_t rate;
    uint16_t id;
    int batchId;
};

class ValveTest
{
private:
    bool flag = false;
    int valveNum;
    int valveLeftIgnore;
    int valveInterval;
    ThreadSafeQueue<ValveTestData> valveTestQueue;

public:
    bool GetFlag() { return flag; }
    // void SetFlag(bool startTestValve_) { startTestValve = startTestValve_; }
    bool IsStop() { return valveTestQueue.empty(); }
    void Push(uint8_t rate, uint16_t id);
    void Clear();

    void Start(int valveNum_, int valveLeftIgnore_, int valveInterval_);
    void Stop() { flag = false; };
    std::vector<BOX_S> ProcessValveTest();
};
