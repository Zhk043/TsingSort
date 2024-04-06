#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include "ts_rne_c_api.h"
#include "ts_rne_log.h"
#include "ts_rne_version.h"
#include "ts_rne_nn_input.h"
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <opencv2/dnn.hpp>
#ifdef __cplusplus
extern "C" {
#endif
#define MAX_DETECT_NUM     1000		// max face detect number

typedef struct ts_BOX_S{
    // the range of x and y is 0.0f~1.0f
    TS_FLOAT f32Xmin;
    TS_FLOAT f32Ymin;
    TS_FLOAT f32Xmax;
    TS_FLOAT f32Ymax;
    TS_FLOAT f32Score;
    TS_FLOAT f32Id;
} BOX_S;
typedef struct ts_DETECT_RESULT_S{
	TS_U32 u32BoxNum;
	BOX_S stBox[MAX_DETECT_NUM];
} DETECT_RESULT_S;
class KNIGHT{
public:
	KNIGHT(std::vector<std::vector<std::vector<float>>>	anchors,int class_num);
	KNIGHT(int class_num);
	KNIGHT();
public:
	bool bAiInit(const char *cpModuleCfgFile,const  char *cpModuleWeightFile,int class_num);
	bool bAiInference(cv::Mat &originImg,DETECT_RESULT_S *pResult);
    bool bAiFree();
	void setNMSParam(float iouTh,float confTh);
	bool bSetCpu(int iCpuId,int iSocket);
	int detectionPostProcess(RNE_BLOB_S *blob,int idx, int start, int cal_width,std::vector<cv::Rect>& boxes,std::vector<float>& confidences,std::vector<int>& classIds);
private:
	RNE_NET_S nnModel;
	TS_S32 num =1;
	RNE_NET_S *net[1] ;
	TS_U8 *paramStride[1];
	cv::Size modelInputSize={640, 640};
	TS_CHAR     *p_cfgBuffer    = NULL;
    TS_CHAR     *p_weightBuffer = NULL;
	int strides[3] = {8,16,32};
	int strides_half[3] = {4,8,16};
    TS_U32 m_NBlob = 0;
	std::vector<std::vector<std::vector<float>>>	m_anchors = {
        {{{13.6015625, 5.3515625}, {17.25, 7.59375}, {19.8125, 10.015625}}},
        {{{24.65625, 8.2109375}, {18.40625, 14.984375}, {25.875, 11.40625}}},
        {{{34.40625, 10.0078125}, {30.59375, 16.15625}, {48.28125, 14.609375}}}
        };
	float iouThre = 0.45;
	float confThre = 0.25;
	int m_class_num = 7;

};
#ifdef __cplusplus
};
#endif
