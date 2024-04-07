//配置文件，用json
#include "Config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "LogManager.h"
#include <unistd.h>
#include <string.h>
#include <iostream>

std::string Config::cmdPath = "./config/command.json";
std::string Config::configPath = "./config/config.json";
std::string Config::matterPath = "./config/matter.json";
Config *Config::instance = nullptr;
std::once_flag Config::m_flag;

Config *Config::GetInstance()
{
    if (instance == nullptr)
    {
        std::call_once(m_flag, CreateInstance);
    }
    return instance;
}

void Config::CreateInstance()
{
    instance = new (std::nothrow) Config();
    if (instance == nullptr)
    {
        throw std::exception();
    }
}

Config::Config()
{
    Init2();
}

Config::~Config()
{
    // cJSON_Delete(configJson);
}

void Config::InitMatter()
{
    matterJson = GetJson(matterPath);
    if (matterJson == nullptr)
    {
        exit(0);
    }

    cJSON *system = cJSON_GetObjectItem(matterJson, "System");
    fpgaNum = cJSON_GetObjectItem(system, "FPGANum")->valueint;
    batchNum = cJSON_GetObjectItem(system, "BatchNum")->valueint;
    forceSynchRowId = cJSON_GetObjectItem(system, "ForceSynchRowId")->valueint;
    onePassNFPGA = cJSON_GetObjectItem(system, "OnePassNFPGA")->valueint;
    oneThreadNReceive = cJSON_GetObjectItem(system, "OneThreadNReceive")->valueint;
    onePacketNRows = cJSON_GetObjectItem(system, "OnePacketNRows")->valueint;
    oneValveNum = cJSON_GetObjectItem(system, "OneValveNum")->valueint;
    valveBlowLineNum = cJSON_GetObjectItem(system, "ValveBlowLineNum")->valueint;
    cJSON *anchorObj = cJSON_GetObjectItem(system, "AnchorSize");
    if (anchorObj != NULL)
    {
        anchorSize = anchorObj->valueint;
    }
    cJSON *backColor = cJSON_GetObjectItem(system, "BackgroundColor");
    backgroundColor.r = cJSON_GetObjectItem(backColor, "r")->valueint;
    backgroundColor.g = cJSON_GetObjectItem(backColor, "g")->valueint;
    backgroundColor.b = cJSON_GetObjectItem(backColor, "b")->valueint;

    cJSON *modelData = cJSON_GetObjectItem(matterJson, "Model");
    int modelNum = cJSON_GetArraySize(modelData);
    models.clear();
    for (int i = 0; i < modelNum; ++i)
    {
        Model model;
        cJSON *modelJson = cJSON_GetArrayItem(modelData, i);
        model.uid = cJSON_GetObjectItem(modelJson, "UId")->valuestring;
        model.name = cJSON_GetObjectItem(modelJson, "Name")->valuestring;
        model.nameCn = cJSON_GetObjectItem(modelJson, "NameCn")->valuestring;
        model.nameEn = cJSON_GetObjectItem(modelJson, "NameEn")->valuestring;
        model.imgRows = cJSON_GetObjectItem(modelJson, "ImageRows")->valueint;
        model.imgCols = cJSON_GetObjectItem(modelJson, "ImageCols")->valueint;
        model.overlayLineNum = cJSON_GetObjectItem(modelJson, "OverlayLineNum")->valueint;
        model.version = cJSON_GetObjectItem(modelJson, "Version")->valuestring;
        model.path = cJSON_GetObjectItem(modelJson, "Path")->valuestring;

        cJSON *categoryNameData = cJSON_GetObjectItem(modelJson, "Classes");
        model.category.num = cJSON_GetArraySize(categoryNameData);
        model.category.names.resize(model.category.num);
        model.category.nameCns.resize(model.category.num);
        model.category.nameEns.resize(model.category.num);
        cJSON *categoryNameCnData = cJSON_GetObjectItem(modelJson, "ClassesCn");
        cJSON *categoryNameEnData = cJSON_GetObjectItem(modelJson, "ClassesEn");
        for (int j = 0; j < model.category.num; ++j)
        {
            model.category.names[j] = cJSON_GetArrayItem(categoryNameData, j)->valuestring;
            model.category.nameCns[j] = cJSON_GetArrayItem(categoryNameCnData, j)->valuestring;
            model.category.nameEns[j] = cJSON_GetArrayItem(categoryNameEnData, j)->valuestring;
        }
        models[model.uid] = model;
        // cJSON_Delete(categoryJson);
    }
}
bool Config::Init2()
{
    // 读取config文件
    cJSON *configJson = GetJson(configPath);
    if (configJson == nullptr)
    {
        exit(0);
    }

    cJSON *valve = cJSON_GetObjectItem(configJson, "Valve");
    valveNum = cJSON_GetObjectItem(valve, "Num")->valueint;
    valveLeftIgnore = cJSON_GetObjectItem(valve, "LeftIgnore")->valueint;
    valveRightIgnore = cJSON_GetObjectItem(valve, "RightIgnore")->valueint;
    valveInterval = cJSON_GetObjectItem(valve, "Interval")->valueint;

    cJSON *networkData = cJSON_GetObjectItem(configJson, "Network");
    cJSON *fpgaAddressData = cJSON_GetObjectItem(networkData, "FPGAAddress");
    cJSON *fpgaLocalAddressData = cJSON_GetObjectItem(networkData, "FPGALocalAddress");
    int fpgaSize = cJSON_GetArraySize(fpgaAddressData);
    fpgaAddresses.resize(fpgaSize);
    fpgaLocalAddresses.resize(fpgaSize);
    for (int i = 0; i < fpgaSize; ++i)
    {
        fpgaAddresses[i] = cJSON_GetArrayItem(fpgaAddressData, i)->valuestring;
        fpgaLocalAddresses[i] = cJSON_GetArrayItem(fpgaLocalAddressData, i)->valuestring;
    }
    fgpaPort = cJSON_GetObjectItem(networkData, "FPGAPort")->valueint;
    fpgaLocalPort = cJSON_GetObjectItem(networkData, "FPGALocalPort")->valueint;

    fpgaRemoteAddress = cJSON_GetObjectItem(networkData, "FPGARemoteAddress")->valuestring;
    screenPort = cJSON_GetObjectItem(networkData, "ScreenPort")->valueint;
    screenLocalPort = cJSON_GetObjectItem(networkData, "ScreenLocalPort")->valueint;
    cJSON_Delete(configJson);
    // 读取matter文件
    InitMatter();
    // cJSON_Delete(matterJson);
    return true;
}

cJSON *Config::GetJson(std::string &path)
{
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
    {
        perror("open config error!");
        return nullptr;
    }
    char buf[20480] = {0}; // 一般不会有这么多个字符

    size_t size = read(fd, buf, sizeof(buf));
    if (size <= 0)
    {
        perror("read config error!");
        close(fd);
        return nullptr;
    }
    close(fd);

    cJSON *json = cJSON_Parse(buf);
    if (json == nullptr)
    {
        perror("parse config error");
        return nullptr;
    }
    return json;
}

std::string Config::GetModelInfo()
{
    InitMatter();
    cJSON *root = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    for (auto pair : models)
    {
        std::string uid = pair.first;
        Model model = pair.second;
        cJSON *object = cJSON_CreateObject();
        cJSON_AddStringToObject(object, "UId", uid.c_str());
        cJSON_AddStringToObject(object, "Name", model.name.c_str());
        cJSON_AddStringToObject(object, "NameCn", model.nameCn.c_str());
        cJSON_AddStringToObject(object, "NameEn", model.nameEn.c_str());
        cJSON_AddStringToObject(object, "Version", model.version.c_str());
        // 标签名称(母语)
        std::vector<const char *> nameptrs;
        for (auto &&name : model.category.names)
        {
            nameptrs.push_back(name.c_str());
        }
        cJSON *labelJson = cJSON_CreateStringArray(nameptrs.data(), model.category.names.size());
        cJSON_AddItemToObject(object, "LabelName", labelJson);
        // 标签名称(中文)
        std::vector<const char *> nameCnptrs;
        for (auto &&nameCn : model.category.nameCns)
        {
            nameCnptrs.push_back(nameCn.c_str());
        }
        cJSON *labelCnJson = cJSON_CreateStringArray(nameCnptrs.data(), model.category.nameCns.size());
        cJSON_AddItemToObject(object, "LabelNameCn", labelCnJson);
        // 标签名称(英文)
        std::vector<const char *> nameEnptrs;
        for (auto &&nameEn : model.category.nameEns)
        {
            nameEnptrs.push_back(nameEn.c_str());
        }
        cJSON *labelEnJson = cJSON_CreateStringArray(nameEnptrs.data(), model.category.nameEns.size());
        cJSON_AddItemToObject(object, "LabelNameEn", labelEnJson);
        cJSON_AddItemToArray(arr, object);
    }
    cJSON_AddItemToObject(root, "Model", arr);
    return cJSON_Print(root);
}

std::string Config::GetParameterInfo()
{
    cJSON *cmdJson = GetJson(cmdPath);
    return cJSON_Print(cmdJson);
}

bool Config::ReadModel(std::string fileName, Model &model)
{
    cJSON *fileJson = GetJson(fileName);
    int cameraNum = cJSON_GetObjectItem(fileJson, "CameraNum")->valueint;
    if (Config::GetInstance()->fpgaNum != cameraNum)
    {
        cJSON_Delete(fileJson);
        return false;
    }
    model.version = cJSON_GetObjectItem(fileJson, "Version")->valuestring;
    model.nameCn = cJSON_GetObjectItem(fileJson, "NameCn")->valuestring;
    model.nameEn = cJSON_GetObjectItem(fileJson, "NameEn")->valuestring;
    model.name = model.nameEn;

    model.imgRows = cJSON_GetObjectItem(fileJson, "ImageRows")->valueint;
    model.imgCols = cJSON_GetObjectItem(fileJson, "ImageCols")->valueint;
    model.overlayLineNum = cJSON_GetObjectItem(fileJson, "OverlayLineNum")->valueint;

    cJSON *classesCnData = cJSON_GetObjectItem(fileJson, "ClassesCn");
    int count = cJSON_GetArraySize(classesCnData);
    model.category.num = count;
    for (int i = 0; i < count; i++)
    {
        std::string nameCn = cJSON_GetArrayItem(classesCnData, i)->valuestring;
        model.category.nameCns.push_back(nameCn);
    }
    cJSON *classesEnData = cJSON_GetObjectItem(fileJson, "ClassesEn");
    count = cJSON_GetArraySize(classesEnData);
    for (int i = 0; i < count; i++)
    {
        std::string nameEn = cJSON_GetArrayItem(classesEnData, i)->valuestring;
        model.category.nameEns.push_back(nameEn);
        model.category.names.push_back(nameEn);
    }
    cJSON_Delete(fileJson);
    return true;
}

bool Config::UpdateModel(Model model)
{
    bool hasModel = false;
    // 标签名称
    std::vector<const char *> nameptrs;
    for (auto &&name : model.category.names)
    {
        nameptrs.push_back(name.c_str());
    }
    // 标签名称(中文)
    std::vector<const char *> nameCnptrs;
    for (auto &&nameCn : model.category.nameCns)
    {
        nameCnptrs.push_back(nameCn.c_str());
    }
    // 标签名称(英文)
    std::vector<const char *> nameEnptrs;
    for (auto &&nameEn : model.category.nameEns)
    {
        nameEnptrs.push_back(nameEn.c_str());
    }
    cJSON *modelData = cJSON_GetObjectItem(matterJson, "Model");
    for (int i = 0; i < cJSON_GetArraySize(modelData); i++)
    {
        cJSON *modelJson = cJSON_GetArrayItem(modelData, i);
        std::string name = cJSON_GetObjectItem(modelJson, "Name")->valuestring;
        std::string version = cJSON_GetObjectItem(modelJson, "Version")->valuestring;
        if (name == model.name && version == model.version)
        {
            hasModel = true;
            // 找到匹配的 Model，替换属性
            cJSON_ReplaceItemInObject(modelJson, "UId", cJSON_CreateString(model.uid.c_str()));
            cJSON_ReplaceItemInObject(modelJson, "Name", cJSON_CreateString(model.name.c_str()));
            cJSON_ReplaceItemInObject(modelJson, "NameCn", cJSON_CreateString(model.nameCn.c_str()));
            cJSON_ReplaceItemInObject(modelJson, "NameEn", cJSON_CreateString(model.nameEn.c_str()));
            cJSON_ReplaceItemInObject(modelJson, "ImageRows", cJSON_CreateNumber(model.imgRows));
            cJSON_ReplaceItemInObject(modelJson, "ImageCols", cJSON_CreateNumber(model.imgCols));
            cJSON_ReplaceItemInObject(modelJson, "OverlayLineNum", cJSON_CreateNumber(model.overlayLineNum));
            cJSON_ReplaceItemInObject(modelJson, "Version", cJSON_CreateString(model.version.c_str()));
            cJSON_ReplaceItemInObject(modelJson, "Path", cJSON_CreateString(model.path.c_str()));
            cJSON_ReplaceItemInObject(modelJson, "Classes", cJSON_CreateStringArray(nameptrs.data(), model.category.names.size()));
            cJSON_ReplaceItemInObject(modelJson, "ClassesCn", cJSON_CreateStringArray(nameCnptrs.data(), model.category.nameCns.size()));
            cJSON_ReplaceItemInObject(modelJson, "ClassesEn", cJSON_CreateStringArray(nameEnptrs.data(), model.category.nameEns.size()));

            break; // 结束循环
        }
    }

    if (!hasModel)
    {
        // 创建新的 Model 项
        cJSON *newModel = cJSON_CreateObject();
        cJSON_AddItemToObject(newModel, "UId", cJSON_CreateString(model.uid.c_str()));
        cJSON_AddItemToObject(newModel, "Name", cJSON_CreateString(model.name.c_str()));
        cJSON_AddItemToObject(newModel, "NameCn", cJSON_CreateString(model.nameCn.c_str()));
        cJSON_AddItemToObject(newModel, "NameEn", cJSON_CreateString(model.nameEn.c_str()));
        cJSON_AddItemToObject(newModel, "ImageRows", cJSON_CreateNumber(model.imgRows));
        cJSON_AddItemToObject(newModel, "ImageCols", cJSON_CreateNumber(model.imgCols));
        cJSON_AddItemToObject(newModel, "OverlayLineNum", cJSON_CreateNumber(model.overlayLineNum));
        cJSON_AddItemToObject(newModel, "Version", cJSON_CreateString(model.version.c_str()));
        cJSON_AddItemToObject(newModel, "Path", cJSON_CreateString(model.path.c_str()));
        cJSON_AddItemToObject(newModel, "Classes", cJSON_CreateStringArray(nameptrs.data(), model.category.names.size()));
        cJSON_AddItemToObject(newModel, "ClassesCn", cJSON_CreateStringArray(nameCnptrs.data(), model.category.nameCns.size()));
        cJSON_AddItemToObject(newModel, "ClassesEn", cJSON_CreateStringArray(nameEnptrs.data(), model.category.nameEns.size()));

        // 将新的 Model 项添加到数组
        cJSON_AddItemToArray(modelData, newModel);
    }

    // int fd = open(matterPath.c_str(), O_RDWR | O_TRUNC);
    // char *buf = cJSON_Print(matterJson);
    // size_t size = write(fd, buf, strlen(buf));
    // free(buf);
    // close(fd);
    // if (size <= 0)
    // {
    //     return false;
    // }
    // return true;
    return FlushMatter();
}

bool Config::DeleteModel(std::string targetUId)
{
    // 删除配置文件中指定模型的信息
    cJSON *modelData = cJSON_GetObjectItem(matterJson, "Model");
    for (int i = 0; i < cJSON_GetArraySize(modelData); i++)
    {
        cJSON *modelJson = cJSON_GetArrayItem(modelData, i);
        std::string uid = cJSON_GetObjectItem(modelJson, "UId")->valuestring;
        if (uid == targetUId)
        {
            cJSON_DeleteItemFromArray(modelData, i);
            break;
        }
    }
    // 删除内存中指定的模型
    // models.erase(targetUId);
    return FlushMatter();
}

bool Config::FlushMatter()
{
    int fd = open(matterPath.c_str(), O_RDWR | O_TRUNC);
    char *buf = cJSON_Print(matterJson);
    size_t size = write(fd, buf, strlen(buf));
    free(buf);
    close(fd);
    if (size <= 0)
    {
        return false;
    }
    return true;
}