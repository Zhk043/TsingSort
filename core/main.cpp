#include <signal.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <string>
#include <chrono>
#include <vector>
#include "Config.h"
#include "SynchProcess.h"
#include "FPGAProcess.h"
#include "DetectProcess.h"
#include "PassProcess.h"
#include <fcntl.h>
#include <thread>
#include "ScreenConnect.h"
#include "LogManager.h"
#include "MemoryPool.h"
#include <signal.h>
#include <execinfo.h>

using namespace std;

std::shared_ptr<FPGAProcess> fpgaProcess;//相机板
std::shared_ptr<SynchProcess> synchProcess;//同步
std::shared_ptr<DetectProcess> detectProcess;//检测进程，处理图像，发送数据
std::shared_ptr<PassProcess> passProcess;//通道进程，界面设置参数
std::shared_ptr<MemoryPool> memoryPool;//内存池，模型切换
std::shared_ptr<KNIGHT> knight;//清微算法库
std::shared_ptr<ScreenConnect> screenConnect;//屏幕的接口

void DealCrash(int num)
{
	size_t size;
	void *array[1024];
	size = backtrace(array, 1024);
	int fd = open("CrashLog.txt", O_WRONLY | O_CREAT | O_APPEND, 0666);
	backtrace_symbols_fd(array, size, fd);
	close(fd);
	exit(1);
}

int main(int argc, char *argv[])
{
	struct sigaction act;
	act.sa_flags = 0;
	act.sa_handler = DealCrash;
	sigemptyset(&act.sa_mask);
	sigaction(SIGILL, &act, nullptr);
	sigaction(SIGSEGV, &act, nullptr);

	const char *process_name = "AiSort";
	prctl(PR_SET_NAME, process_name);
	int devId = 1;
	if (argc > 1)
	{
		devId = atoi(argv[1]);
	}
	auto config = Config::GetInstance();
	string versionStr = "version: " + std::to_string(First_Version_Id) + "." + std::to_string(Second_Version_Id);
	std::cout << versionStr << std::endl;
#ifdef OPEN_LOG
	CLogInfo(versionStr);
#endif

	// 读取IP地址
	// std::map<int, int> ipMapFpgaIds;
	std::string addr = config->fpgaRemoteAddress;
	int ip_num[4] = {0};
	sscanf(addr.c_str(), "%d.%d.%d.%d", &ip_num[0], &ip_num[1], &ip_num[2], &ip_num[3]);
	int currFpgaIp = ip_num[3];
	// ipMapFpgaIds[currFpgaIp] = devId;

	//实例化对象
	Model firstModel = config->models.begin()->second;
	int imgRows = firstModel.imgRows;
	int imgCols = firstModel.imgCols;
	int overlayLineNum = firstModel.overlayLineNum;
	int valveNum = config->valveNum;
	int bgColorR = config->backgroundColor.r;
	int bgColorG = config->backgroundColor.g;
	int bgColorB = config->backgroundColor.b;

	memoryPool = std::make_shared<MemoryPool>(imgRows, imgCols, 1, valveNum, 1, overlayLineNum, bgColorR, bgColorG, bgColorB);

	fpgaProcess = std::make_shared<FPGAProcess>(currFpgaIp);
	synchProcess = std::make_shared<SynchProcess>();
	fpgaProcess->SetSynchProcess(synchProcess);

	detectProcess = std::make_shared<DetectProcess>();
	passProcess = std::make_shared<PassProcess>();

	// 创建目标检测对象
	std::vector<std::vector<std::vector<float>>> anchors = {
		{{{10, 13}, {16, 30}, {33, 23}}},
		{{{30, 61}, {62, 45}, {59, 119}}},
		{{{116, 90}, {156, 198}, {373, 326}}}};
	if (config->anchorSize == 2)
	{
		anchors = {{{{20, 26}, {32, 60}, {66, 46}}},
				   {{{2.5, 3}, {4, 7}, {7.5, 5.5}}}};
	}
	knight = std::make_shared<KNIGHT>(anchors, 4);
	knight->setNMSParam(0.6, 0.25);

	std::thread detectTh(&DetectProcess::DetectImage, detectProcess);
	detectTh.detach();

	sleep(1);
	std::thread receiveTh(&FPGAProcess::ReceiveImage, fpgaProcess);//创建接收图像的线程
	receiveTh.detach();

	// ping相机板
	string ping = "ping -c 1 " + addr;
	system(ping.c_str());

	screenConnect = std::make_shared<ScreenConnect>(devId, currFpgaIp);
	EventManager::GetInstance()->RegisterEventCallback(screenConnect); // 注册文件导出回调
	screenConnect->StartConnect();									   //  阻塞于此
	return 0;								   //  阻塞于此
}
