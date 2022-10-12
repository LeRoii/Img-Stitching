#include <termio.h>
#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>

#include "imageProcess.h"
#include "nvcam.hpp"
#include "PracticalSocket.h"
#include "ocvstitcher.hpp"
#include "helper_timer.h"
// #include "nvrender.h"
#include "nvrenderAlpha.h"

using namespace cv;

static int g_keyboardinput = 0;

static char buffer[SLAVE_PCIE_UDP_BUF_LEN]; // Buffer for echo string

static vector<Mat> upImgs(4);
vector<Mat> downImgs(4);
vector<Mat> stitcherOut(2);
Mat upRet, downRet, ret;
int framecnt = 0;

bool saveret = false;
bool detect = false;
bool initonline = false;
bool start_ssr = false;
bool savevideo = false;
bool displayori = false;
int videoFps = 10;

static int imgcut = 0;
static std::string net;
static std::string canname;
static stCamCfg ymlCameraCfg;
static std::string cameraParamsPath;
static bool websocketOn;
static int websocketPort;
static std::string stitchercfgpath = "/home/nx/code/release/stitcher-imx390cfg.yaml";

static bool
parse_cmdline(int argc, char **argv)
{
    std::cout<<"Help: use 's' to save image, use 'd' to detect, use 'i' to online init, use 'h' to open HDR!"<<std::endl;
    int c;

    if (argc < 2)
    {
        return true;
    }
    string cfgpath;
    while ((c = getopt(argc, argv, "sdievoc:")) != -1)
    {
        switch (c)
        {
            case 's':
                saveret = true;
                break;
            case 'd':
                detect = true;
                break;
            case 'i':
                initonline = true;
                break;
            case 'e':
                start_ssr = true;
                break;
            case  'v':
                savevideo = true;
                break;
            case 'o':
                displayori = true;
                break;
            case 'c':
                cfgpath = optarg;
                spdlog::info("cfg path:{}", cfgpath);
                if(std::string::npos == cfgpath.find(".yaml"))
                    spdlog::warn("input cfgpath invalid, use default");
                else
                    stitchercfgpath = cfgpath;
                break;
            default:
                break;
        }
    }
}


static int parseYml()
{
    try
    {
        YAML::Node config = YAML::LoadFile(stitchercfgpath);
        cameraParamsPath = config["cameraparams"].as<string>();
        ymlCameraCfg.camSrcWidth = config["camsrcwidth"].as<int>();
        ymlCameraCfg.camSrcHeight = config["camsrcheight"].as<int>();
        ymlCameraCfg.distoredWidth = config["distorWidth"].as<int>();
        ymlCameraCfg.distoredHeight = config["distorHeight"].as<int>();
        ymlCameraCfg.undistoredWidth = config["undistorWidth"].as<int>();
        ymlCameraCfg.undistoredHeight = config["undistorHeight"].as<int>();
        ymlCameraCfg.outPutWidth = config["outPutWidth"].as<int>();
        ymlCameraCfg.outPutHeight = config["outPutHeight"].as<int>();
        ymlCameraCfg.undistor = config["undistor"].as<bool>();
        ymlCameraCfg.vendor = config["vendor"].as<string>();
        ymlCameraCfg.sensor = config["sensor"].as<string>();
        ymlCameraCfg.fov = config["fov"].as<int>();

        imgcut = config["imgcut"].as<int>();
        num_images = config["num_images"].as<int>();

        renderWidth = config["renderWidth"].as<int>();
        renderHeight = config["renderHeight"].as<int>();
        renderX = config["renderX"].as<int>();
        renderY = config["renderY"].as<int>();
        renderBufWidth = config["renderBufWidth"].as<int>();
        renderBufHeight = config["renderBufHeight"].as<int>();

        USED_CAMERA_NUM = config["USED_CAMERA_NUM"].as<int>();
        net = config["netpath"].as<string>();

        canname = config["canname"].as<string>();
        renderMode = config["renderMode"].as<int>();

        stitcherMatchConf = config["stitcherMatchConf"].as<float>();
        stitcherAdjusterConf = config["stitcherAdjusterConf"].as<float>();
        stitcherBlenderStrength = config["stitcherBlenderStrength"].as<float>();
        stitcherCameraExThres = config["stitcherCameraExThres"].as<float>();
        stitcherCameraInThres = config["stitcherCameraInThres"].as<float>();

        batchSize = config["batchSize"].as<int>();
        initMode = config["initMode"].as<int>();

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

        weburi = config["websocketurl"].as<string>();
        websocketOn = config["websocketOn"].as<bool>();
        websocketPort = config["websocketPort"].as<int>();

        detect = config["detection"].as<bool>();
    }
    catch(...)
    {
        spdlog::critical("yml parse failed, check your config yaml!");
        return RET_ERR;
    }

    return RET_OK;
}

imageProcessor *nvProcessor = nullptr;
jetsonEncoder *encoder = nullptr;

int main(int argc, char *argv[])
{
    if(RET_ERR == parse_cmdline(argc, argv))
        return RET_ERR;

    if(RET_ERR == parseYml())
        return RET_ERR;

    // nvrenderCfg rendercfg{renderBufWidth, renderBufHeight, renderWidth, renderHeight, renderX, renderY, renderMode};
    // nvrenderAlpha *renderer = new nvrenderAlpha(rendercfg);

    stCamCfg camcfgs[CAMERA_NUM];
    for(int i=0;i<CAMERA_NUM;i++)
    {
        camcfgs[i] = ymlCameraCfg;
        camcfgs[i].id = i;
        std::string str = "/dev/video";
        strcpy(camcfgs[i].name, (str+std::to_string(i)).c_str());
    }

    static std::shared_ptr<nvCam> cameras[CAMERA_NUM];
    
    for(int i=0;i<USED_CAMERA_NUM;i++)
    {
        cameras[i].reset(new nvCam(camcfgs[i]));
        if(RET_ERR == cameras[i]->init(stitchercfgpath))
        {
            spdlog::warn("camera [{}] init failed!", i);
        }
    }

    std::vector<std::thread> threads;
    for(int i=0;i<USED_CAMERA_NUM;i++)
        threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
    for(auto& th:threads)
        th.detach();

    nvProcessor = new imageProcessor(); 
    nvProcessor->init(stitchercfgpath);
    encoder = new jetsonEncoder(websocketOn, websocketPort);

    stStitcherCfg stitchercfg[2] = {stStitcherCfg{ymlCameraCfg.outPutWidth, ymlCameraCfg.outPutHeight, 
                            0, num_images, stitcherMatchConf, stitcherAdjusterConf, stitcherBlenderStrength, 
                            stitcherCameraExThres, stitcherCameraInThres, stitchercfgpath, initMode},
                                    stStitcherCfg{ymlCameraCfg.outPutWidth, ymlCameraCfg.outPutHeight, 
                                    1, num_images, stitcherMatchConf, stitcherAdjusterConf, stitcherBlenderStrength, 
                                    stitcherCameraExThres, stitcherCameraInThres, stitchercfgpath, initMode}};

    ocvStitcher ostitcherUp(stitchercfg[0]);
    ocvStitcher ostitcherDown(stitchercfg[1]);

    ostitcherUp.init();
    ostitcherDown.init();

    int failnum = 0;
    do{
        upImgs.clear();
        for(int i=0;i<num_images;i++)
        {
            cameras[i]->getFrame(upImgs[i], false);
        }

        failnum++;
        if(failnum > 5)
        {
            spdlog::warn("initalization failed due to environment, use default parameters");
            initMode = 2;
        }
    }
    while(ostitcherUp.init(upImgs, initMode) != 0);
    spdlog::info("up init ok!!!!!!!!!!!!!!!!!!!!");

    do{
        downImgs.clear();
        for(int i=0;i<num_images;i++)
        {
            cameras[i+num_images]->getFrame(downImgs[i], false);
        }

        failnum++;
        if(failnum > 5)
        {
            spdlog::warn("initalization failed due to environment, use default parameters");
            initMode = 2;
        }
    }
    while(ostitcherDown.init(downImgs, initMode) != 0);
    spdlog::info("down init ok!!!!!!!!!!!!!!!!!!!!");

    StopWatchInterface *timer = NULL;
    sdkCreateTimer(&timer);
    sdkResetTimer(&timer);
    sdkStartTimer(&timer);

    while(1)
    {
        spdlog::debug("start loop");
        sdkResetTimer(&timer);
        
        cameras[0]->getFrame(upImgs[0], false);
        cameras[1]->getFrame(upImgs[1], false);
        cameras[2]->getFrame(downImgs[0], false);
        cameras[3]->getFrame(downImgs[1], false);

        spdlog::info("read takes:{} ms", sdkGetTimerValue(&timer));

        std::thread t1 = std::thread(&ocvStitcher::process, &ostitcherUp, std::ref(upImgs), std::ref(stitcherOut[0]));
        std::thread t2 = std::thread(&ocvStitcher::process, &ostitcherDown, std::ref(downImgs), std::ref(stitcherOut[1]));

        t1.join();
        t2.join();
        spdlog::info("stitching takes:{} ms", sdkGetTimerValue(&timer));

        cv::resize(stitcherOut[0], stitcherOut[0], stitcherOut[1].size());

        cv::Mat up,down,ori;

        cv::vconcat(stitcherOut[0], stitcherOut[1], ret);
        cv::rectangle(ret, cv::Rect(0, ret.rows/2-5, ret.cols, 10), cv::Scalar(0,0,0), -1);
  
        spdlog::debug("ret size:[{},{}]", ret.size().width, ret.size().height);
        spdlog::info("concat takes:{} ms", sdkGetTimerValue(&timer));

        if(detect)
        {
            ret = nvProcessor->ProcessOnce(ret);
            spdlog::debug("detect takes:{} ms", sdkGetTimerValue(&timer));
        }

        static bool fitsizeok = false;
        cv::Mat final;
        // if(!fitsizeok)
        // {
        //     int maxWidth = 1280;
        //     double fitscale = 1;
        //     if(ret.cols > maxWidth)
        //     {
        //         fitscale = maxWidth * 1.0 / ret.cols;
        //     }
        //     cv::resize(ret, final, cv::Size(), fitscale, fitscale);
        //     fitsizeok = true;
        // }
        // cv::resize(ret, final, cv::Size(), fitscale, fitscale);

        if(websocketOn)
            // encoder->process(final); 
            encoder->sendBase64(final); 
        
        spdlog::debug("frame [{}], render takes:{} ms", framecnt, sdkGetTimerValue(&timer));

        spdlog::info("frame [{}], all takes:{} ms", framecnt++, sdkGetTimerValue(&timer));

    }
    return 0;
}