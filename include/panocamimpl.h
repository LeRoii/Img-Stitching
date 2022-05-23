#ifndef _PANOCAMIMPL_H_
#define _PANOCAMIMPL_H_

#include "stitcherglobal.h"
#include "yaml-cpp/yaml.h"
#include "nvcam.hpp"
#include "ocvstitcher.hpp"
#include "PracticalSocket.h"
#include "imageProcess.h"
#include "spdlog/spdlog.h"
#include "canmessenger.hpp"

class panocamimpl
{
public:
    panocamimpl(std::string yamlpath);
    ~panocamimpl() = default;

    int init();
    int getCamFrame(int id, cv::Mat &frame);
    int getPanoFrame(cv::Mat &ret);
    int detect(cv::Mat &img, std::vector<int> &ret);
    int detect(cv::Mat &img);
    int imgEnhancement(cv::Mat &img);
    int render(cv::Mat &img);
    int drawCross(cv::Mat &img);
    int drawCamCross(cv::Mat &img);
    int saveAndSend(cv::Mat &img);
    bool verify();
    uint8_t getStatus();
    int sendStatus();
    stSysStatus& sysStatus();

private:
    std::shared_ptr<nvCam> cameras[CAMERA_NUM];
    std::shared_ptr<ocvStitcher> stitchers[2];
    imageProcessor *pImgProc; 
    // nvrender *pRenderer;
    int finalcut;
    int framecnt;
    unsigned char m_StatusCode;
    canmessenger *pCANMessenger;
    stSysStatus m_stSysStatus;
};


#endif