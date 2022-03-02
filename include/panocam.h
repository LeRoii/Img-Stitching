#ifndef _PANOCAM_H_
#define _PANOCAM_H_

#include <memory>
#include <opencv2/opencv.hpp>

class __attribute__((visibility("default"))) panocam
{
public:
    panocam(std::string yamlpath);
    ~panocam();
    int init();
    // int captureFrames();
    int getCamFrame(int id, unsigned char *pData, unsigned int nDataSize);
    int getCamFrame(int id, cv::Mat &frame);
    int getPanoFrame(cv::Mat &ret);
    int detect(cv::Mat &img, std::vector<int> &ret);
    int detect(cv::Mat &img);
    int imgEnhancement(cv::Mat &img);
    int render(cv::Mat &img);

private:
    class panocamimpl;
    std::unique_ptr<panocamimpl> pimpl;
};

#endif