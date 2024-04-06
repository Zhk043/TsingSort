#include "ValveTest.h"

void ValveTest::Start(int valveNum_, int valveLeftIgnore_, int valveInterval_)
{
    flag = true;
    valveNum = valveNum_;
    valveLeftIgnore = valveLeftIgnore_;
    valveInterval = valveInterval_;
}

void ValveTest::Push(uint8_t rate, uint16_t id)
{
    if (id > valveNum * Config::GetInstance()->oneValveNum)
    {
        std::cout << "Screen send error valve id: " << id << std::endl;
        return;
    }
    valveTestQueue.push(ValveTestData(rate, id, 0));
}

std::vector<BOX_S> ValveTest::ProcessValveTest()
{
    std::vector<BOX_S> blowObjects;
    ValveTestData valveTestData;
    bool hasData = valveTestQueue.try_pop(valveTestData);
    if (!hasData)
        return blowObjects;

    BOX_S obj;
    obj.f32Xmin = valveLeftIgnore + valveInterval * (valveTestData.id - 1);
    obj.f32Xmax = obj.f32Xmin + 2;
    obj.f32Ymin = 1;

    int height = 32;
    if (valveTestData.rate > 0)
    {
        height = 64;
    }
    obj.f32Ymax = obj.f32Ymin + height;
    blowObjects.push_back(obj);
    return blowObjects;
}

void ValveTest::Clear()
{
    valveTestQueue.Clear();
}
