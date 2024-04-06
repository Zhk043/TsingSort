#include "ImageBatchSaving.h"

std::string ImageBatchSaving::GetCurrentTimeString()
{
    struct timeval tv;
    char buf[64];
    gettimeofday(&tv, NULL);
    strftime(buf, sizeof(buf) - 1, "%Y%m%d%H%M%S", localtime(&tv.tv_sec)); // 根据要求时间格式化
    std::string usec = std::to_string(tv.tv_usec);
    if (usec.length() == 4) // keep str at same length
        usec = "00" + usec;
    else if (usec.length() == 5)
        usec = "0" + usec;
    strcat(buf, usec.c_str());
    std::string str(buf);
    return str;
}

void ImageBatchSaving::SetFlag(uint32_t flag, uint32_t mode)
{
    savedImgCount = 0;
    saveImgFlag = flag > 0;
    saveSymbolImgFlag = mode > 0;
}

void ImageBatchSaving::SaveImg(cv::Mat &img, const DETECT_RESULT_S &result, const std::vector<std::string> &categoryNames)
{
    cv::Mat src;
    cv::cvtColor(img, src, cv::COLOR_RGB2BGR);

    std::string imageName = GetCurrentTimeString();
    std::string filename_src = "./dataset/" + imageName + ".bmp";
    cv::imwrite(filename_src, src);

    if (saveSymbolImgFlag)
    {
        std::string filename = "./dataset/" + imageName + ".jpg";
        for (size_t i = 0; i < result.u32BoxNum; i++)
        {
            BOX_S obj = result.stBox[i];
            auto pt1 = cv::Point(obj.f32Xmin, obj.f32Ymin);
            auto pt2 = cv::Point(obj.f32Xmax, obj.f32Ymax);
            cv::rectangle(src, pt1, pt2, cv::Scalar(0x27, 0xC1, 0x36), 1);

            auto pt_category = obj.f32Ymin > 15 ? pt1 : pt2;
            auto pt_score = obj.f32Ymin > 15 ? cv::Point(obj.f32Xmin + 40, obj.f32Ymin) : cv::Point(obj.f32Xmax + 40, obj.f32Ymax);
            cv::putText(src, categoryNames[obj.f32Id].substr(0, 3), pt_category, cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0xFF, 0xFF, 0xFF), 1);
            cv::putText(src, std::to_string((int)(obj.f32Score * 100)), pt_score, cv::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(0x00, 0x00, 0xFF), 1);
        }
        cv::imwrite(filename, src); // 保存图片，标记吹出的目标
    }
    savedImgCount++;
}
