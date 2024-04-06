#include <iostream>
#include <fstream>
#include <algorithm>
#include "Config.h"
#include <string>
#include <sys/prctl.h>

bool FileExists(const std::string &filename)
{
    std::ifstream file(filename);
    return file.good();
}

std::string GetFileMd5(std::string fileName)
{
    std::string cmd = "md5sum ";
    cmd += fileName + std::string(" | cut -d ' ' -f1");
    FILE *pf = popen(cmd.c_str(), "r");
    char buf[1024] = {0};
    fread(buf, sizeof(buf), 1, pf);
    pclose(pf);
    // 去除换行符
    std::string res(buf);
    if (!res.empty() && res[res.length() - 1] == '\n')
    {
        res.erase(res.length() - 1);
    }
    return res;
}

int main(int argc, char **argv)
{
    char *process_name = "deploy_model";
    prctl(PR_SET_NAME, process_name);

    if (argc < 2)
    {
        std::cout << "deploy input parameter error" << std::endl;
        return 0;
    }
    std::string modelName = std::string(argv[1]);
    std::string fileName = "deploy/" + modelName + ".json";
    if (!FileExists(fileName))
    {
        std::cout << "file does not exist" << std::endl;
        return 0;
    }

    Model model;
    model.uid = GetFileMd5(fileName);
    model.path = "models/" + modelName;

    auto config = Config::GetInstance();
    if (!config->ReadModel(fileName, model))
    {
        std::cout << "model read error" << std::endl;
        return 0;
    }

    // 若叠行输入不合法进行合理性替换
    if ((model.overlayLineNum < 0) || (model.overlayLineNum >= model.imgRows))
    {
        int overlayNum = 32;
        for (auto pair : config->models)
        {
            Model models = pair.second;
            if (models.name == model.name && models.version == model.version)
            {
                overlayNum = models.overlayLineNum;
                break;
            }
        }
        model.overlayLineNum = overlayNum;
    }
    std::cout << "overlayLineNum: " << model.overlayLineNum << ", batchSize: " << config->batchNum << std::endl;

    if (!config->UpdateModel(model))
    {
        std::cout << "update model error!" << std::endl;
        return 0;
    }
    return 1;
}
