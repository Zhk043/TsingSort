#include "PassProcess.h"
#include "Config.h"
#include <string>

using namespace std;
using namespace cv;

PassProcess::PassProcess()
{
    globalImgCut = cv::Rect(0, 0, 0, 0);
    halfImgCut = cv::Rect(0, 0, 0, 0);
    memset(globalImgCutTmp, 0, sizeof(globalImgCutTmp));
    memset(halfImgCutTmp, 0, sizeof(halfImgCutTmp));
    labelConfs.resize(30, 1);
    isEnable = false;
    matterRatio = 0;
    filterArea = 0;
    labelIndex = -1;
    cutoff = 0;
    lowerLimitHeight = 0;
}

void PassProcess::SetGlobalImgCut(u32 dir, u32 value)
{
    globalImgCutTmp[dir] = (int)value - 100;
    switch (dir)
    {
    case 0:
        globalImgCut.y = globalImgCutTmp[0];
        globalImgCut.height = -(globalImgCutTmp[0] + globalImgCutTmp[1]);
        break;
    case 1:
        globalImgCut.height = -(globalImgCutTmp[0] + globalImgCutTmp[1]);
        break;
    case 2:
        globalImgCut.x = globalImgCutTmp[2];
        globalImgCut.width = -(globalImgCutTmp[2] + globalImgCutTmp[3]);
        break;
    case 3:
        globalImgCut.width = -(globalImgCutTmp[2] + globalImgCutTmp[3]);
        break;
    default:
        break;
    }
}

void PassProcess::SetGlobalImgCutByPercent(u32 dir, u32 value)
{
    globalImgCutTmp[dir] = (int)value - 100;
}

void PassProcess::SetHalfImgCut(u32 dir, u32 value)
{
    halfImgCutTmp[dir] = (int)value - 100;
    switch (dir)
    {
    case 0:
        halfImgCut.y = halfImgCutTmp[0];
        halfImgCut.height = -(halfImgCutTmp[0] + halfImgCutTmp[1]);
        break;
    case 1:
        halfImgCut.height = -(halfImgCutTmp[0] + halfImgCutTmp[1]);
        break;
    case 2:
        halfImgCut.x = halfImgCutTmp[2];
        halfImgCut.width = -(halfImgCutTmp[2] + halfImgCutTmp[3]);
        break;
    case 3:
        halfImgCut.width = -(halfImgCutTmp[2] + halfImgCutTmp[3]);
        break;
    default:
        break;
    }
}

void PassProcess::SetEnable(u32 value)
{
    isEnable = value > 0;
    std::cout << "####isEnable is: " << isEnable << std::endl;
}

void PassProcess::SetConf(u32 conf, int id)
{
    std::cout << "conf " << id << " is " << conf << std::endl;
    if (id < (int)labelConfs.size())
    {
        labelConfs[id] = 1.0 - (float)(conf) * 1.0 / 100;
    }
}

void PassProcess::SetConf(std::vector<int> confs)
{
    for (size_t i = 0; i < confs.size(); i++)
    {
        std::cout << "####conf is: " << confs[i] << std::endl;
        labelConfs[i] = 1.0 - confs[i] * 1.0 / 100;
    }
}

void PassProcess::SetMatterRatio(u32 value)
{
    matterRatio = (float)(value) * 1.0 / 10;
}

void PassProcess::SetFilterArea(u32 value)
{
    filterArea = value;
}

void PassProcess::SetLabelId(u32 value)
{
    labelIndex = value - 1;
}

void PassProcess::SetCutoff(u32 value)
{
    cutoff = value;
}

void PassProcess::SetLowerLimitHeight(u32 value)
{
    lowerLimitHeight = value;
}

void PassProcess::DealObjRectByPercent(BOX_S &obj)
{
    // 上下左右按比例缩放
    int halfHeight = (obj.f32Ymax - obj.f32Ymin) / 2;
    obj.f32Ymax = obj.f32Ymax - halfHeight * globalImgCutTmp[1] / 100;
    obj.f32Ymin = obj.f32Ymin + halfHeight * globalImgCutTmp[0] / 100;

    int halfWidth = (obj.f32Xmax - obj.f32Xmin) / 2;
    obj.f32Xmax = obj.f32Xmax - halfWidth * globalImgCutTmp[3] / 100;
    obj.f32Xmin = obj.f32Xmin + halfWidth * globalImgCutTmp[2] / 100;
}

void PassProcess::DealDetectObj(cv::Mat &src, const BOX_S &obj, std::vector<BOX_S> &objs, int labelNum)
{
    if (!isEnable)
        return; // 该通道算法使能没有打开

    // if (labelIndex != -1 && labelIndex < labelNum)
    // {
    //     if (obj.f32Ymin > cutoff && obj.f32Ymax < src.rows - cutoff && (int)obj.f32Id == labelIndex)
    //     {
    //         float height = obj.f32Ymax - obj.f32Ymin;
    //         float width = obj.f32Xmax - obj.f32Xmin;
    //         float ratio = max(height, width) / min(height, width);
    //         if (ratio < matterRatio)
    //         {
    //             objs.push_back(obj);
    //         }
    //     }
    // }

    if (obj.f32Score > labelConfs[(int)obj.f32Id])
    {
        objs.push_back(obj);
    }
}
