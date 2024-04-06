#pragma once

#include "cJSON.h"
#include <string>
#include <mutex>
#include <vector>
#include <map>

struct Category
{
    int num;
    std::vector<std::string> names;
    std::vector<std::string> nameCns;
    std::vector<std::string> nameEns;
};

struct Model
{
    std::string uid;
    std::string name;
    std::string nameCn;
    std::string nameEn;
    int imgRows;
    int imgCols;
    int overlayLineNum;
    std::string version;
    std::string path;
    Category category;
};

struct Color
{
    int r;
    int g;
    int b;
};

class Config
{
public:
    static Config *GetInstance();
    static void CreateInstance();
    Config();
    ~Config();
    bool Init2();
    std::string GetModelInfo();
    std::string GetParameterInfo();
    bool UpdateModel(Model model);
    bool ReadModel(std::string fileName, Model &model);
    bool DeleteModel(std::string targetUId);

public:
    int valveNum;
    int valveLeftIgnore; // 左边忽略像素
    int valveRightIgnore;
    int valveInterval; // 喷阀间间隔
    int fpgaNum;       // 一个工控机对应几个fpga
    int batchNum;
    int onePassNFPGA;      // 一个通道对应几个fpga
    int oneThreadNReceive; // 一个接受图片线程接受几个相机板的数据
    int onePacketNRows;    // 相机板一个数据包发多少行数据
    int oneValveNum;       // 单个阀的阀口数量
    int valveBlowLineNum;  // 阀数据每次下发的行数
    int anchorSize = 3;    //用于选择模型训练时不同的anchor参数
    std::vector<std::string> fpgaAddresses;
    std::vector<std::string> fpgaLocalAddresses;
    int fgpaPort;
    int fpgaLocalPort;
    std::string fpgaRemoteAddress;
    int screenPort;
    int screenLocalPort;
    std::map<std::string, Model> models;

    Color backgroundColor;
    int forceSynchRowId;

private:
    cJSON *GetJson(std::string &path);
    // bool creatConfig(Category classes, std::string path);
    void InitMatter();
    bool FlushMatter();

private:
    static std::string cmdPath;
    static std::string configPath;
    static std::string matterPath;
    cJSON *configJson;
    cJSON *matterJson;

    static Config *instance;
    static std::once_flag m_flag;
};