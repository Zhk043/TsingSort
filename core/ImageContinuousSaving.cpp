//屏端的连续采图
#include "ImageContinuousSaving.h"

void ImageContinuousSaving::Start(uint32_t type_, int imgRows_, int imgCols_, int overlayLineNum_)
{
    imgRows = imgRows_;
    imgCols = imgCols_;
    overlayLineNum = overlayLineNum_;
    continuousImg = cv::Mat(ContiImgRow, imgCols, CV_8UC3);

    flag = true;
    type = type_;
    completedRowId = 0;
    isSkipCurrImg = true;
}

void ImageContinuousSaving::SaveImg(const cv::Mat &img)
{
    if (isSkipCurrImg) // 为了防止采到的部分图像数据是缓存中的之前图像数据，直接跳过本张图像采下一张
    {
        isSkipCurrImg = false;
        return;
    }
    if (completedRowId >= ContiImgRow)
        return;
    int imgRowsTmp = imgRows;
    int indexAdd = 0;
    if (completedRowId > 0)
    {
        imgRowsTmp = imgRows - overlayLineNum;
        indexAdd = overlayLineNum;
    }
    for (int i = 0; i < imgRowsTmp && i + completedRowId < ContiImgRow; ++i)
    {
        for (int j = 0; j < imgCols; ++j)
        {
            continuousImg.at<cv::Vec3b>(completedRowId + i, j) = img.at<cv::Vec3b>(i + indexAdd, j);
        }
    }
    completedRowId += imgRowsTmp;
}

bool ImageContinuousSaving::GetImg(uint32_t requestRowId, uint8_t *buf)
{
    if (requestRowId >= ContiImgRow || buf == nullptr)
        return false;
    if (requestRowId >= completedRowId)
        return false;
    for (int i = 0; i < continuousImg.cols; ++i)
    {
        buf[i * 3] = continuousImg.at<cv::Vec3b>(requestRowId, i)[0];
        buf[i * 3 + 1] = continuousImg.at<cv::Vec3b>(requestRowId, i)[1];
        buf[i * 3 + 2] = continuousImg.at<cv::Vec3b>(requestRowId, i)[2];
    }
    return true;
}
