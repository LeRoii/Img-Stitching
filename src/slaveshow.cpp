#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>
#include "yaml-cpp/yaml.h"
#include "nvcam.hpp"
#include "PracticalSocket.h"
#include "nvrender.h"
#include "stitcherglobal.h"
#include "imageProcess.h"
#include "helper_timer.h"

using namespace cv;

vector<Mat> upImgs(4);
vector<Mat> downImgs(4);
Mat upRet, downRet;

vector<Mat> imgs(CAMERA_NUM);

std::string cfgpath;
std::string defaultcfgpath = "/home/nvidia/cfg/slaveshow.yaml";
int framecnt = 0;

bool detect = false;


imageProcessor *nvProcessor = nullptr;

int main(int argc, char *argv[])
{

    YAML::Node config = YAML::LoadFile(defaultcfgpath);
    camSrcWidth = config["camsrcwidth"].as<int>();
    camSrcHeight = config["camsrcheight"].as<int>();

    renderWidth = config["renderWidth"].as<int>();
    renderHeight = config["renderHeight"].as<int>();
    renderX = config["renderX"].as<int>();
    renderY = config["renderY"].as<int>();

    undistorWidth = 960;
    undistorHeight = 540;
    stitcherinputWidth = 480;
    stitcherinputHeight = 270;


    std::string net = config["netpath"].as<string>();

    std::string loglvl = config["loglvl"].as<string>();
    if(loglvl == "critical")
        spdlog::set_level(spdlog::level::critical);
    else if(loglvl == "trace")
        spdlog::set_level(spdlog::level::trace);
    else if(loglvl == "warn")
        spdlog::set_level(spdlog::level::warn);
    else if(loglvl == "info")
        spdlog::set_level(spdlog::level::info);
    else
        spdlog::set_level(spdlog::level::debug);

    USED_CAMERA_NUM = 3;

    stNvrenderCfg rendercfg{renderBufWidth, renderBufHeight, renderWidth, renderHeight, renderX, renderY, renderMode};
    nvrender *renderer = new nvrender(rendercfg);

    imgs = std::vector<Mat>(CAMERA_NUM, Mat(stitcherinputHeight, stitcherinputWidth, CV_8UC4));
    
    if (detect)
        nvProcessor = new imageProcessor(net);


    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,1,"/dev/video1", vendor},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,2,"/dev/video2", vendor},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,3,"/dev/video3", vendor},
                                    };

    std::shared_ptr<nvCam> cameras[CAMERA_NUM];

    for(int i=0;i<USED_CAMERA_NUM;i++)
            cameras[i].reset(new nvCam(camcfgs[i]));

    std::vector<std::thread> threads;
    for(int i=0;i<USED_CAMERA_NUM;i++)
        threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
    for(auto& th:threads)
        th.detach();

    Mat rets[USED_CAMERA_NUM];

    StopWatchInterface *timer = NULL;
    sdkCreateTimer(&timer);
    sdkResetTimer(&timer);
    sdkStartTimer(&timer);

    cv::Mat ret = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);;
    cv::Mat hret,vret;

    while(1)
    {
        sdkResetTimer(&timer);
        

        for(int i=0;i<USED_CAMERA_NUM;i++)
        {
            // cameras[i]->read_frame();
            cameras[i]->getFrame(imgs[i], false);
        }

        cv::vconcat(vector<cv::Mat>{imgs[0], imgs[1], imgs[2]}, hret);
        cv::hconcat(vector<cv::Mat>{imgs[0], imgs[1], imgs[2]}, vret);

        
        hret.copyTo(ret(cv::Rect(0,135, 480, 810)));
        vret.copyTo(ret(cv::Rect(480,405, 1440, 270)));

        spdlog::info("frame [{}], read takes:{} ms", framecnt, sdkGetTimerValue(&timer));
        
        cv::Mat yoloret;// = ret.clone();
        // cv::Mat yoloret = ret(cv::Rect(640, 300, 640, 480)).clone();
        if (detect)
        {
            yoloret = nvProcessor->ProcessOnce(ret);
        }

        renderer->showImg(ret);
        
        spdlog::info("frame [{}], all takes:{} ms", framecnt++, sdkGetTimerValue(&timer));

    }
    return 0;
}