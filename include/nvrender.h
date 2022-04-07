#ifndef _NVRENDER_H_
#define _NVRENDER_H_

#include <opencv2/opencv.hpp>
#include "NvEglRenderer.h"
#include "nvbuf_utils.h"
#include "spdlog/spdlog.h"
#include "stitcherglobal.h"

static int offsetX, offsetY, h, w;
static double fitscale;
class nvrender
{
public:
    nvrender(nvrenderCfg cfg);
    ~nvrender();
    void render(unsigned char *data);
    void drawIndicator();
    void fit2final(cv::Mat &input, cv::Mat &output);
    void renderegl(cv::Mat &img);
    void renderocv(cv::Mat &img, cv::Mat &final);
    void render(cv::Mat &img);
    void render(cv::Mat &img, cv::Mat &final);
    void showImg(cv::Mat &img);
    void showImg();

private:
    NvEglRenderer *renderer;
    int nvbufferfd;
    int nvbufferWidth, nvbufferHeight;
    cv::Mat canvas;
    int m_mode;
    int maxHeight, maxWidth;
    int longStartX, uplongStartY, uplongEndY, upshortEndY;
    int downlongStartY, downlongEndY, downshortEndY;
    int indicatorStartX;
};

#endif