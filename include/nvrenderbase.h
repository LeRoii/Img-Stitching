#ifndef _NVRENDER_H_
#define _NVRENDER_H_

#include <opencv2/opencv.hpp>
#include "NvEglRenderer.h"
#include "nvbuf_utils.h"
#include "spdlog/spdlog.h"
#include "stitcherglobal.h"

// static int offsetX, offsetY, h, w;
// static double fitscale;
class nvrenderbase
{
public:
    nvrenderbase(const stNvrenderCfg &cfg);
    virtual ~nvrenderbase();
    virtual void drawIndicator() = 0;
    virtual cv::Mat render(cv::Mat &img) = 0;
    virtual void showImg(cv::Mat &img) = 0;

protected:
    virtual cv::Mat renderegl(cv::Mat &img) = 0;
    virtual cv::Mat renderocv(cv::Mat &img) = 0;

    NvEglRenderer *renderer;
    int nvbufferfd;
    int nvbufferWidth, nvbufferHeight;
    cv::Mat canvas;
    int m_mode;
    int maxHeight, maxWidth;
    int longStartX, uplongStartY, uplongEndY, upshortEndY, uplongLen, upshortLen;
    int shortStep, longStep;
    int downlongStartY, downlongEndY, downshortEndY;
    int indicatorStartX;
    int oriStartX, oriStartY, panoMargin, panoHeight, panoWidth;
};

#endif