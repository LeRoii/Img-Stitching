#ifndef _PANOCAM_H_
#define _PANOCAM_H_

#include <memory>
#include <opencv2/opencv.hpp>

class __attribute__((visibility("default"))) panocam
{
public:
    panocam();
    ~panocam();
    int init();
    // int captureFrames();
    int getCamFrame(int id, unsigned char *pData, unsigned int nDataSize);
    int getCamFrame(int id, cv::Mat &frame);

private:
    class panocamimpl;
    std::unique_ptr<panocamimpl> pimpl;

};

#endif