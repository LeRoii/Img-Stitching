#include "panocam.h"
#include "nvcam.hpp"
#include "stitcherconfig.h"
#include "spdlog/spdlog.h"

class panocam::panocamimpl
{
public:
    panocamimpl()
    {
        stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,1,"/dev/video0"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,2,"/dev/video1"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,3,"/dev/video2"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,4,"/dev/video3"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,5,"/dev/video4"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,6,"/dev/video5"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,7,"/dev/video7"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,8,"/dev/video6"}};

        for(int i=0;i<USED_CAMERA_NUM;i++)
            cameras[i].reset(new nvCam(camcfgs[i]));

    }
    
    ~panocamimpl() = default;

    int init()
    {
        spdlog::info("init");
                
        return RET_OK;
    }

    int getCamFrame(int id, cv::Mat &frame)
    {
        return RET_OK;
    }

private:
    std::shared_ptr<nvCam> cameras[USED_CAMERA_NUM];
};

panocam::panocam():pimpl{std::make_unique<panocamimpl>()}
{

}

panocam::~panocam() = default;

int panocam::init()
{
    pimpl->init();
}

int panocam::getCamFrame(int id, cv::Mat &frame)
{
    return pimpl->getCamFrame(id, frame);
}