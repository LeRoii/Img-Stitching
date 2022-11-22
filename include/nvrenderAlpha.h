#ifndef _NVRENDERALPHA_H_
#define _NVRENDERALPHA_H_

#include "nvrenderbase.h"

class nvrenderAlpha : public nvrenderbase
{
public:
    nvrenderAlpha(const stNvrenderCfg &cfg);
    ~nvrenderAlpha();

    void drawIndicator();
    
    cv::Mat render(cv::Mat &img);
    // void render(cv::Mat &img, cv::Mat &final);
    void showImg(cv::Mat &img);
    void renderimgs(cv::Mat &img, cv::Mat &inner, int x, int y);

private:
    cv::Mat renderegl(cv::Mat &img);
    cv::Mat renderocv(cv::Mat &img);
    cv::Mat fit2final(cv::Mat &input);
};

#endif