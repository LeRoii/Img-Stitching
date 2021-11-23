#ifndef _PANOCAM_H_
#define _PANOCAM_H_

#include <memory>
#include <opencv2/opencv.hpp>

enum enInitMode
{
    INIT_ONLINE = 1,
    INIT_OFFLINE = 2
};

class __attribute__((visibility("default"))) panocam
{
public:
    panocam();
    ~panocam();
    int init(enInitMode mode);
    // int captureFrames();
    int getCamFrame(int id, unsigned char *pData, unsigned int nDataSize);
    int getCamFrame(int id, cv::Mat &frame);
    int getPanoFrame(cv::Mat &ret);

private:
    class panocamimpl;
    std::unique_ptr<panocamimpl> pimpl;

};

#endif