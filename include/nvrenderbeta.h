#ifndef _NVRENDERALPHA_H_
#define _NVRENDERALPHA_H_

#include "nvrenderbase.h"

class nvrenderBeta : public nvrenderbase
{
public:
    nvrenderBeta(const stNvrenderCfg &cfg);
    ~nvrenderBeta();

    void drawIndicator();
    cv::Mat render(cv::Mat &img);
    void showImg(cv::Mat &img);
    void renderimgs(cv::Mat &img, cv::Mat &inner, int x, int y);
    void renderWithUi(cv::Mat &pano, cv::Mat &ori);

private:
    cv::Mat renderegl(cv::Mat &img);
    cv::Mat renderocv(cv::Mat &img);    
    void fit2final(cv::Mat &input, cv::Mat &output);
};

#endif