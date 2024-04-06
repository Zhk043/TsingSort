#include "ScreenConnect.h"
#include "Config.h"
#include "LogManager.h"
#include <sys/time.h>
#include "DetectProcess.h"
#include "PassProcess.h"
#include "SynchProcess.h"
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sys/stat.h>
#include "MemoryPool.h"

extern std::shared_ptr<SynchProcess> synchProcess;
extern std::shared_ptr<DetectProcess> detectProcess;
extern std::shared_ptr<PassProcess> passProcess;
extern std::shared_ptr<MemoryPool> memoryPool;

void ScreenConnect::HandleCmd(u8 *data, u16 data_len)
{
    YLS_ENET_FRAME packet;
    memset(&packet, 0, sizeof(packet));

    packet.boardId = data[0];
    packet.operatorID = data[1];
    packet.startAddress = ((u32)data[2] << 24) + ((u32)data[3] << 16) + ((u32)data[4] << 8) + ((u32)data[5]);
    packet.len = ((u16)data[6] << 8) + ((u16)data[7]);
    packet.packetId = ((u16)data[8] << 8) + ((u16)data[9]);
    packet.rev1 = data[10];
    packet.rev2 = ((u32)data[11]) + ((u32)data[12] << 8) + ((u32)data[13] << 16) + ((u32)data[14] << 24);
    // 对包中数据段进行解析
    if (packet.len > 1) // 内部约定命令数据段最多4个字节
    {
        // packet.Dbuffer[0] = ((u32)data[15] << 24) + ((u32)data[16] << 16) + ((u32)data[17] << 8) + ((u32)data[18]);
        packet.Bbuffer[0] = data[18];
        packet.Bbuffer[1] = data[17];
        packet.Bbuffer[2] = data[16];
        packet.Bbuffer[3] = data[15];
    }
    else
    {
        packet.Bbuffer[0] = data[15];
    }

    u16 dataLen = 0;
    if (packet.operatorID == 0xB0)
    {
        switch (packet.startAddress)
        {
        case 0x00000104: // 查询AI程序通信质量
            dataLen = 1;
            packet.Bbuffer[0] = 0;
            break;

        case 0x00000134: // 获取版本号
        {
            std::cout << "##134## version: " << First_Version_Id << "." << Second_Version_Id << std::endl;
            dataLen = 4;
            u16 firstVersionId = First_Version_Id;
            u16 secondVersionId = Second_Version_Id;
            packet.Bbuffer[0] = (u8)(firstVersionId >> 8);
            packet.Bbuffer[1] = (u8)firstVersionId;
            packet.Bbuffer[2] = (u8)(secondVersionId >> 8);
            packet.Bbuffer[3] = (u8)secondVersionId;
            break;
        }
        case 0x00000140: // 获取连续采图的图像
        {
            u32 rowId = packet.Dbuffer[0];
            dataLen = memoryPool->GetImgCols() * 3;
            if (!detectProcess->GetContinuousImg(rowId, packet.Bbuffer))
            {
                dataLen = 1;
                packet.Bbuffer[0] = 0xFF;
            }
        }
        break;
        case 0x00000146: // 获取CPU温度
        {
            dataLen = 1;
            int temp = GetCPUTemp();
            packet.Bbuffer[0] = temp < 0 ? 0 : ((u8)temp);
            std::cout << "##146## CPU Temperature: " << (int)packet.Bbuffer[0] << std::endl;
        }
        break;
        case 0x00000149: // 获取波形图
        {
            dataLen = memoryPool->GetImgCols() * 3;
            synchProcess->GetSomeImageData(packet.Bbuffer, 96);
            packet.Bbuffer[0] = packet.Bbuffer[6];
            packet.Bbuffer[1] = packet.Bbuffer[7];
            packet.Bbuffer[2] = packet.Bbuffer[8];
            packet.Bbuffer[3] = packet.Bbuffer[6];
            packet.Bbuffer[4] = packet.Bbuffer[7];
            packet.Bbuffer[5] = packet.Bbuffer[8];
        }
        break;
        case 0x00000151: // 调试信息
        {
            std::cout << "##151## Get Debug Info" << std::endl;
            std::string info = "Debug Info from AI";
            dataLen = info.length() + 1;
            memcpy(packet.Bbuffer, info.c_str(), dataLen - 1);
        }
        break;
        case 0x00000155: // 模型信息
        {
            std::cout << "##155## Get Model Info" << std::endl;
            std::string modelsInfo = Config::GetInstance()->GetModelInfo();
            dataLen = modelsInfo.length() + 1;
            memcpy(packet.Bbuffer, modelsInfo.c_str(), dataLen - 1);
        }
        break;
        case 0x00000161: // 自定义参数信息
        {
            std::cout << "##161## Get Parameter Info" << std::endl;
            std::string paraInfo = Config::GetInstance()->GetParameterInfo();
            dataLen = paraInfo.length() + 1;
            memcpy(packet.Bbuffer, paraInfo.c_str(), dataLen - 1);
        }
        break;
        case 0x00000162: // dataset文件数量
        {
            std::cout << "##162## Get image count" << std::endl;
            dataLen = 4;
            packet.Dbuffer[0] = htonl(GetFileNum("dataset/"));
        }
        break;
        case 0x00000163: // dataset文件列表
        {
            u8 pageSize = packet.rev1;
            u32 pageIndex = packet.rev2;
            std::cout << "##163## Get image list, pageIndex: " << pageIndex << ", pageSize: " << (int)pageSize << std::endl;
            string imageNames = GetImageNamesInfo("dataset/", pageSize, pageIndex);
            dataLen = imageNames.length();
            memcpy(packet.Bbuffer, imageNames.c_str(), dataLen);
        }
        break;
        case 0x00000164: // 文件导出开始
        {
            std::cout << "##164## File download";
            char fileName[1024] = {0}; // 长度不能超过1024
            for (int i = 0; i < packet.len; ++i)
            {
                fileName[i] = data[YLS_NET_HEADER + i];
            }
            std::cout << ", name: " << fileName << std::endl;
            string filePathName = "dataset/" + string(fileName);
            int size = 0;
            struct stat statBuf;
            if (stat(filePathName.c_str(), &statBuf) == 0)
            {
                std::cout << "File total size(byte): " << statBuf.st_size << std::endl;
                size = statBuf.st_size / 1024; // kb
            }

            // packet.Dbuffer[0] = htonl(size);
            string md5 = GetFileMd5(filePathName);
            string info = GetFragmentInfo(fileName, size, md5.c_str());
            dataLen = info.length() + 1;
            memcpy(packet.Bbuffer, info.c_str(), dataLen - 1);

            std::cout << "##165## File Export..." << std::endl;
            // std::thread t(&ScreenConnect::Export, this, filePathName, packet.boardId);
            // t.detach();
            std::shared_ptr<Event> exportEvent = std::make_shared<ExportEvent>(filePathName, packet.boardId);
            EventManager::GetInstance()->TriggerEvent(exportEvent);
        }
        break;
        }
    }
    else if (packet.operatorID == 0xB1)
    {
        switch (packet.startAddress)
        {
        case 0x00000008: // 关机
            system("shutdown -h now");
            break;

        case 0x0000000C: // 重启
            system("reboot");
            break;
        case 0x00000114: // 批量采图 预留字节1：保留标记后图片，0：不保留，包后第一个u32数据1：开始采图，0：停止采图
            std::cout << "##114## Batch Save Image: " << (int)packet.Dbuffer[0] << std::endl;
            detectProcess->SetSaveImgFlag(packet.Dbuffer[0], packet.rev2);
            if (packet.Dbuffer[0] > 0)
            {
                system("rm -rf dataset/*");
                OpenCamera();
            }
            else
            {
                StopCamera();
            }
            break;
        case 0x0000012C: // 设置长宽比
            std::cout << "##12C## SetMatterRatio: " << (int)packet.Dbuffer[0] << std::endl;
            passProcess->SetMatterRatio(packet.Dbuffer[0]);
            break;
        case 0x0000014A: // 设置过滤面积
            std::cout << "##14A## SetFilterArea: " << (int)packet.Dbuffer[0] << std::endl;
            passProcess->SetFilterArea(packet.Dbuffer[0]);
            break;
        case 0x00000108: // 算法使能
            std::cout << "##108## AI Enable: " << (int)packet.Dbuffer[0] << std::endl;
            passProcess->SetEnable(packet.Dbuffer[0]);
            break;
        case 0x00000130: // 系统开关，吹阀使能
            std::cout << "##130## ValveEnable: " << (int)packet.Dbuffer[0] << std::endl;
            detectProcess->SetValveEnable(packet.Dbuffer[0]);
            if (packet.Dbuffer[0] > 0)
            {
                OpenCamera();
                started = true;
            }
            else
            {
                started = false;
                StopCamera();
            }
            break;
        case 0x00000138: // 采连续图开始
        {
            std::cout << "##138## start screen image" << std::endl;
            detectProcess->StartSaveContinuousImg(packet.Bbuffer[2]);
            OpenCamera();
        }
        break;
        case 0x0000013C: // 采连续图结束
        {
            std::cout << "##13C## stop screen image" << std::endl;
            detectProcess->StopSaveContinuousImg();
            StopCamera();
        }
        break;
        case 0x00000141: // 设置上下左右压缩的吹气像素，全局的
            std::cout << "GlobalImgCut: " << packet.rev2 << " value: " << packet.Dbuffer[0] << std::endl;
            passProcess->SetGlobalImgCutByPercent(packet.rev2, packet.Dbuffer[0]);
            break;
        case 0x00000142: // 设置上下左右压缩的吹气像素，局部
            std::cout << "HalfImgCut: " << packet.rev2 << " value: " << packet.Dbuffer[0] << std::endl;
            passProcess->SetHalfImgCut(packet.rev2, packet.Dbuffer[0]);
            break;
        case 0x00000156: // 模型切换
        {
            char uid[1024] = {0};
            for (int i = 0; i < packet.len; ++i)
            {
                uid[i] = data[YLS_NET_HEADER + i];
            }
            std::cout << "####156## value: " << uid << std::endl;
            if (packet.len == 0 || (Config::GetInstance()->models).count(uid) <= 0)
            {
                std::cout << "warning: uid is invalid" << std::endl;
                break;
            }
            detectProcess->ModifyModelId(uid);
        }
        break;
        case 0x00000144: // 喷阀测试
        {
            u16 id = packet.Bbuffer[2]; // 使用2个字节，第1个和第3个表示id
            id = (id << 8) + packet.Bbuffer[0];
            std::cout << "##144## id: " << (int)id << " rate: " << (int)packet.Bbuffer[1] << std::endl;
            detectProcess->TestValve(packet.Bbuffer[1], id);
        }
        break;
        case 0x00000145: // 时间同步
        {
            time_t t = packet.Dbuffer[0];
            std::cout << "##145## time set: " << asctime(localtime(&t)) << std::endl;
            struct timeval tv;
            tv.tv_sec = t;
            int res = settimeofday(&tv, NULL);
            if (res == -1)
            {
                perror("time set error");
            }
        }
        break;
        case 0x00000159: // 升级包文件接收
        {
            // std::cout << "##159## recv update package " << packet.rev2 << std::endl;
            dataLen = 1;
            if (fragmentId != 0 && fragmentId + 1 != packet.rev2)
            {
                break;
            }
            fragmentId = packet.rev2;
            if (fragmentId == 0)
            {
                pkgFd = open("/root/work/install.ai", O_CREAT | O_TRUNC | O_RDWR, 0666);
                if (pkgFd == -1)
                {
                    perror("fail to open the file");
                    break;
                }
            }
            int res = write(pkgFd, data + YLS_NET_HEADER, packet.len);
            if (res != -1)
            {
                packet.Bbuffer[0] = 0x01;
            }
            if (packet.rev1 == 1)
            {
                close(pkgFd);
                fragmentId = 0;
            }
        }
        break;
        case 0x00000147: // 程序升级
        {
            std::cout << "##147## program update" << std::endl;
            std::string filePath = "/root/work/install.ai";
            system("rm -rf deploy/*");
            std::string command = std::string("unzip -j -o -d deploy/ ") + filePath;
            std::cout << command << std::endl;
            system(command.c_str());
            system((std::string("rm -rf ") + filePath).c_str());
            system("sh deploy/install.sh");
            system("sync");
            sleep(1);
            // system("reboot");
        }
        break;
        case 0x00000148: // 喷阀测试开/关
            std::cout << "##148## valve: " << packet.Dbuffer[0] << std::endl;
            if (packet.Dbuffer[0] > 0)
            {
                detectProcess->StartTestValve();
                OpenCamera();
            }
            else
            {
                detectProcess->StopTestValve();
                StopCamera();
            }
            break;
        case 0x00000153: // 上下截断值
            std::cout << "##153## Cutoff: " << packet.Dbuffer[0] << std::endl;
            passProcess->SetCutoff(packet.Dbuffer[0]);
            break;
        case 0x00000154: // 高度下限值
            std::cout << "##154## LowerLimitHeight: " << packet.Dbuffer[0] << std::endl;
            passProcess->SetLowerLimitHeight(packet.Dbuffer[0]);
            break;
        case 0x00000160: // 调试信息写
            system("sh debug.sh");
            break;
        case 0x00000168: // 设置生效物料类别
            std::cout << "##168## SetLabelId: " << (int)packet.Dbuffer[0] << std::endl;
            passProcess->SetLabelId(packet.Dbuffer[0]);
            break;
        case 0x0000016B: // 模型删除
        {
            char uid[1024] = {0};
            for (int i = 0; i < packet.len; ++i)
            {
                uid[i] = data[YLS_NET_HEADER + i];
            }
            std::cout << "##16B## Delete Model UId: " << uid << std::endl;
            if (packet.len == 0 || (Config::GetInstance()->models).count(uid) <= 0)
            {
                std::cout << "warning: uid is invalid" << std::endl;
                break;
            }
            Config::GetInstance()->DeleteModel(uid);
        }
        break;
        default:
            if (packet.startAddress >= 0x00000300) // 设置置信度，0x300开始
            {
                int id = (int)(packet.startAddress - 0x00000300);
                passProcess->SetConf(packet.Dbuffer[0], id);
            }
            break;
        }
    }

    packet.len = (dataLen >> 8) + (dataLen << 8);
    packet.startAddress = htonl(packet.startAddress);
    packet.packetId = htons(packet.packetId);
    packet.rev2 = packet.rev2;

    SendCommand((u8 *)&packet, YLS_NET_HEADER + dataLen);
}

ScreenConnect::ScreenConnect(int devId_, int currFpgaIp_)
    : devId(devId_), currFpgaIp(currFpgaIp_), EventCallback(EventType::Export)
{
    if ((screenSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket() error");
        return;
    }
    sockaddr_in localAddr;
    bzero(&localAddr, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &localAddr.sin_addr);
    localAddr.sin_port = htons(Config::GetInstance()->screenLocalPort);

    if (bind(screenSocket, (struct sockaddr *)(&localAddr), sizeof(localAddr)) < 0)
    {
        perror("screen socket bind local address error");
        exit(1);
    }

    bzero(&screenAddr, sizeof(screenAddr));
    screenAddr.sin_family = AF_INET;
    inet_pton(AF_INET, Config::GetInstance()->fpgaRemoteAddress.c_str(), &screenAddr.sin_addr);
    screenAddr.sin_port = htons(Config::GetInstance()->fgpaPort);
}

int ScreenConnect::ReceiveCommand(unsigned char *buf, int size)
{
    return recvfrom(screenSocket, buf, size, 0, NULL, NULL);
}

int ScreenConnect::SendCommand(unsigned char *buf, int len)
{
    int n = sendto(screenSocket, buf, len, 0, (sockaddr *)(&screenAddr), sizeof(screenAddr));

    if (n < 0)
    {
        perror("send error");
    }
    return n;
}

int ScreenConnect::GetCPUTemp()
{
    char raw[32];
    int fdr = open("/sys/bus/iio/devices/iio:device0/in_voltage0_raw", O_RDONLY);
    if (fdr < 0 || read(fdr, raw, 32) < 0)
    {
        std::cout << "get cpu in_voltage0_raw error!" << std::endl;
    }
    close(fdr);
    char scale[32];
    int fds = open("/sys/bus/iio/devices/iio:device0/in_voltage_scale", O_RDONLY);
    if (fds < 0 || read(fds, scale, 32) < 0)
    {
        std::cout << "get cpu in_voltage_scale error!" << std::endl;
    }
    close(fds);

    double voltage = atof(raw) * atof(scale);
    double resistance = 10 * voltage / (1800 - voltage);
    std::cout << "resistance: " << resistance << std::endl;
    double temp = -21.61 * std::log(resistance) + 127.33;
    return temp;
}

int ScreenConnect::GetFileNum(char *folderPath)
{
    int cnt = 0;
    for (auto &file : std::filesystem::directory_iterator(folderPath))
    {
        if (file.is_regular_file())
            cnt++;
    }
    return cnt;
}

vector<string> ScreenConnect::GetFileNames(char *folderPath, int pageSize, int pageIndex)
{
    int skippedCnt = 0;
    vector<string> files;
    for (auto &file : filesystem::directory_iterator(folderPath))
    {
        if (file.is_regular_file())
        {
            skippedCnt++;
            if (pageIndex != 0 && pageSize != 0 && skippedCnt < pageIndex * pageSize)
            {
                continue;
            }
            string fileName = file.path().filename().string();
            files.push_back(fileName);
            if (pageSize != 0 && files.size() == pageSize)
            {
                break;
            }
        }
    }
    return files;
}

string ScreenConnect::GetImageNamesInfo(char *folderPath, int pageSize, int pageIndex)
{
    cJSON *root = cJSON_CreateObject();
    vector<string> fileNames = GetFileNames(folderPath, pageSize, pageIndex);
    if (fileNames.size() == 0)
        return "";
    std::vector<const char *> nameptrs;
    for (auto &&name : fileNames)
    {
        nameptrs.push_back(name.c_str());
    }
    cJSON *nameJson = cJSON_CreateStringArray(nameptrs.data(), fileNames.size());
    cJSON_AddItemToObject(root, "images", nameJson);
    return cJSON_Print(root);
}

string ScreenConnect::GetFragmentInfo(char *fileName, int size, const char *data)
{
    cJSON *object = cJSON_CreateObject();
    cJSON_AddStringToObject(object, "Name", fileName);
    // cJSON_AddStringToObject(object, "CTime", asctime(localtime(&ctime)));
    cJSON_AddNumberToObject(object, "Size", size);
    // cJSON_AddNumberToObject(object, "IsLast", isLast);
    // cJSON_AddNumberToObject(object, "FragmentId", fragId);
    // cJSON_AddNumberToObject(object, "FragmentSize", fragSize);
    cJSON_AddStringToObject(object, "Md5", data);
    return cJSON_Print(object);
}

void ScreenConnect::StartConnect()
{
    while (true)
    {
        unsigned char buf[NET_BUFFER_SIZE] = {0};
        int packetSize = ReceiveCommand(buf, sizeof(buf)); // 屏程序的命令参数
        if (packetSize >= YLS_NET_HEADER)
            HandleCmd(buf, packetSize);
    }
}

void ScreenConnect::OpenCamera()
{
#ifndef TSING_SINGLE_NIC
    if (started)
        return;
    YLS_ENET_FRAME packet;
    memset(&packet, 0, sizeof(packet));
    packet.boardId = currFpgaIp;
    packet.operatorID = 0x03;
    packet.startAddress = 0x00000020;
    packet.len = 4;
    packet.Dbuffer[0] = 0x01000000; // 上传数据使能

    packet.startAddress = htonl(packet.startAddress);
    packet.len = htons(packet.len);
    packet.packetId = htons(packet.packetId);
    packet.rev2 = htonl(packet.rev2);
    SendCommand((u8 *)&packet, YLS_NET_HEADER + 4);
#endif
}

void ScreenConnect::StopCamera()
{
#ifndef TSING_SINGLE_NIC
    if (started)
        return;
    YLS_ENET_FRAME packet;
    memset(&packet, 0, sizeof(packet));
    packet.boardId = currFpgaIp;
    packet.operatorID = 0x03;
    packet.startAddress = 0x00000020;
    packet.len = 4;
    packet.Dbuffer[0] = 0x00000000; // 上传数据禁止
    packet.startAddress = htonl(packet.startAddress);
    packet.len = htons(packet.len);
    packet.packetId = htons(packet.packetId);
    packet.rev2 = htonl(packet.rev2);
    SendCommand((u8 *)&packet, YLS_NET_HEADER + 4);
#endif
}

void ScreenConnect::Execute(std::shared_ptr<Event> event)
{
    std::shared_ptr<ExportEvent> eventTmp = std::dynamic_pointer_cast<ExportEvent>(event);
    Export(eventTmp->fileName_, eventTmp->boardId_);
}

void ScreenConnect::Export(string fileName, u8 boardId)
{
    YLS_ENET_FRAME packet;
    memset(&packet, 0, sizeof(packet));
    packet.boardId = boardId;
    packet.operatorID = 0xB0;
    packet.startAddress = htonl(0x00000165);
    packet.packetId = 0;

    ifstream in(fileName, ios::binary);
    const int FRAG_SIZE = 8000; // 相机板单次传输上限8k(8096)
    char dataBuf[FRAG_SIZE] = {0};
    int fragId = 0;
    int isLast = 0;
    struct stat statBuf;
    stat(fileName.c_str(), &statBuf);
    int size = std::ceil(statBuf.st_size * 1.0 / FRAG_SIZE);
    int fragSize = FRAG_SIZE;
    while (in.read(dataBuf, fragSize))
    {
        fragSize = in.gcount();
        // std::cout << "fragId:" << fragId << " fragSize: " << fragSize << std::endl;
        if (fragId == size - 1)
            isLast = 1;
        memcpy(packet.Bbuffer, dataBuf, fragSize);
        packet.len = htons(fragSize);
        packet.rev1 = (u8)isLast;
        packet.rev2 = htonl(fragId);
        SendCommand((u8 *)&packet, YLS_NET_HEADER + fragSize);

        fragId++;
        usleep(1000);
    }

    fragSize = in.gcount();
    if (fragSize > 0)
    {
        // std::cout << "fragId:" << fragId << " fragSize: " << fragSize << std::endl;
        memcpy(packet.Bbuffer, dataBuf, fragSize);
        packet.len = htons(fragSize);
        packet.rev1 = 1;
        packet.rev2 = htonl(fragId);
        SendCommand((u8 *)&packet, YLS_NET_HEADER + fragSize);
    }

    in.close();
}

string ScreenConnect::GetFileMd5(string fileName)
{
    string cmd = "md5sum ";
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
