
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


// #define CAMERA_NUM 8
// #define USED_CAMERA_NUM 6
// #define BUF_LEN 65540 

using namespace cv;





static unsigned short servPort = 10001;
static UDPSocket sock(servPort);

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


std::string stitchercfgpath = "../cfg/stitcher-imx390cfg.yaml";

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

static int detCamNum = 0;
static void OnMouseAction(int event, int x, int y, int flags, void *data)
{
    if(event==CV_EVENT_LBUTTONDOWN)
    {
        int ancientDetCamNum = detCamNum;
        // cv::Point* pt = static_cast<cv::Point*>(data);
        spdlog::critical("CV_EVENT_LBUTTONDOWN::[{},{}]", x, y);
        int height = ret.size().height;
        int width = ret.size().width;
        if(y<height/2)
        {
            if(x < width/4)
                detCamNum =  4;
            else if(x < width/2)
                detCamNum =  3;
            else if(x < width *3/4)
                detCamNum =  2;
            else
                detCamNum =  1;
        }
        else
        {
             if(x < width/4)
                detCamNum =  8;
            else if(x < width/2)
                detCamNum =  7;
            else if(x < width *3/4)
                detCamNum =  6;
            else
                detCamNum =  5;
        }
        
        // if(ancientDetCamNum == detCamNum)
        //     return;
        // if(ancientDetCamNum != 0)
        //     cameras[ancientDetCamNum-1]->setDistoredSize(960);
        // if(detCamNum > 6)
        // {
        //     spdlog::warn("src not available");
        //     detCamNum = 0;
        //     return;
        // }
        // cameras[detCamNum-1]->setDistoredSize(1920);
    }
}

imageProcessor *nvProcessor = nullptr;

int main(int argc, char *argv[])
{
    if(RET_ERR == parse_cmdline(argc, argv))
        return RET_ERR;

    YAML::Node config = YAML::LoadFile(stitchercfgpath);

    std::string cameraParamsPath = config["cameraparams"].as<string>();
    stCamCfg ymlCameraCfg;

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

    int imgcut = config["imgcut"].as<int>();
    num_images = config["num_images"].as<int>();

    renderWidth = config["renderWidth"].as<int>();
    renderHeight = config["renderHeight"].as<int>();
    renderX = config["renderX"].as<int>();
    renderY = config["renderY"].as<int>();
    renderBufWidth = config["renderBufWidth"].as<int>();
    renderBufHeight = config["renderBufHeight"].as<int>();

    USED_CAMERA_NUM = config["USED_CAMERA_NUM"].as<int>();
    std::string net = config["netpath"].as<string>();
    std::string cfgpath = config["camcfgpath"].as<string>();
    std::string canname = config["canname"].as<string>();
    // undistor = config["undistor"].as<bool>();
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

    int finalcut = 15;
    if(stitcherinputWidth == 480)
        finalcut = 15;
    else if(stitcherinputWidth == 640)
        finalcut = 40;
    else if(stitcherinputWidth == 720)
        finalcut = 35;

    nvrenderCfg rendercfg{renderBufWidth, renderBufHeight, renderWidth, renderHeight, renderX, renderY, renderMode};
    nvrenderAlpha *renderer = new nvrenderAlpha(rendercfg);

    // stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,1,"/dev/video0", vendor},
    //                                 stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,2,"/dev/video1", vendor},
    //                                 stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,3,"/dev/video2", vendor},
    //                                 stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,4,"/dev/video3", vendor},
    //                                 stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,5,"/dev/video4", vendor},
    //                                 stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,6,"/dev/video5", vendor},
    //                                 stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,7,"/dev/video6", vendor},
    //                                 stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,8,"/dev/video7", vendor}};

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
        cameras[i]->init(cameraParamsPath);
    }

    std::vector<std::thread> threads;
    for(int i=0;i<USED_CAMERA_NUM;i++)
        threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
    for(auto& th:threads)
        th.detach();

    nvProcessor = new imageProcessor(net, canname, batchSize);  


    /************************************stitch all *****************************************/
    {
    // vector<Mat> imgs(8);

    // ocvStitcher stitcherall(stitcherinputWidth, stitcherinputHeight, 3);

    // vector<Mat> imgss;
    // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/1.png"));
    // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/2.png"));
    // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/3.png"));
    // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/4.png"));
    // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/5.png"));
    // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/6.png"));
    // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/7.png"));
    // // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/8.png"));

    // do{
    //     imgs.clear();
    //     for(int i=0;i<USED_CAMERA_NUM;i++)
    //     {
    //         cameras[i]->read_frame();
    //         imgs.push_back(cameras[i]->m_ret);
    //     }
    //     serverCap();
    //     imgs.push_back(downImgs[2]);
    //     imgs.push_back(downImgs[3]);

        
    // }
    // while(stitcherall.init(imgs, initonline) != 0);

    // // ocvStitcher ostitcherUp(960/2, 540/2, 1);
    // // ocvStitcher ostitcherDown(960/2, 540/2, 2);
    // // ocvStitcher ostitcherUp(stitcherinputWidth, stitcherinputHeight, 1);
    // // ocvStitcher ostitcherDown(stitcherinputWidth, stitcherinputHeight, 2);

    // std::vector<std::thread> threads;
    // for(int i=0;i<USED_CAMERA_NUM;i++)
    //     threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
    // for(auto& th:threads)
    //     th.detach();

    // while(1)
    // {
    //     Mat ret;
    //     imgs.clear();
    //     spdlog::debug("start loop");
    //     auto t = cv::getTickCount();
    //     auto all = cv::getTickCount();

    //     std::thread server(serverCap);
    //     cameras[0]->getFrame(imgs[0]);
    //     cameras[1]->getFrame(imgs[1]);
    //     cameras[2]->getFrame(imgs[2]);
    //     cameras[3]->getFrame(imgs[3]);
    //     cameras[4]->getFrame(imgs[4]);
    //     cameras[5]->getFrame(imgs[5]);
    //     imgs[6] = downImgs[2];
    //     imgs[7] = downImgs[3];

    //     // imgss.clear();
    //     // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/1.png"));
    //     // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/2.png"));
    //     // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/3.png"));
    //     // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/4.png"));
    //     // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/5.png"));
    //     // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/6.png"));
    //     // imgss.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/7.png"));

    //     spdlog::debug("master cap fini");
    //     server.join();
    //     spdlog::debug("slave cap fini");

    //     spdlog::info("read takes : {:03.3f} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);
    //     t = cv::getTickCount();

    //     stitcherall.process(imgs, ret);

    //     imshow("ret", ret);
    //     waitKey(1);
    //     spdlog::info("******all takes: {:03.3f} ms", ((getTickCount() - all) / getTickFrequency()) * 1000);
    // }
    }
    /************************************stitch all end*****************************************/

    stStitcherCfg stitchercfg[2] = {stStitcherCfg{ymlCameraCfg.outPutWidth, ymlCameraCfg.outPutHeight, 
                            0, num_images, stitcherMatchConf, stitcherAdjusterConf, stitcherBlenderStrength, 
                            stitcherCameraExThres, stitcherCameraInThres, stitchercfgpath},
                                    stStitcherCfg{ymlCameraCfg.outPutWidth, ymlCameraCfg.outPutHeight, 
                                    1, num_images, stitcherMatchConf, stitcherAdjusterConf, stitcherBlenderStrength, 
                                    stitcherCameraExThres, stitcherCameraInThres, stitchercfgpath}};

    ocvStitcher ostitcherUp(stitchercfg[0]);
    ocvStitcher ostitcherDown(stitchercfg[1]);

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

	VideoWriter *panoWriter = nullptr;
	VideoWriter *oriWriter = nullptr;
    bool writerInit = false;

    StopWatchInterface *timer = NULL;
    sdkCreateTimer(&timer);
    sdkResetTimer(&timer);
    sdkStartTimer(&timer);
    
    while(1)
    {
        spdlog::debug("start loop");
        sdkResetTimer(&timer);
        
        // cameras[0]->getFrame(upImgs[0], false);
        cameras[0]->getFrame(upImgs[0], false);
        cameras[1]->getFrame(upImgs[1], false);
        cameras[2]->getFrame(downImgs[0], false);
        cameras[3]->getFrame(downImgs[1], false);

        cv::Mat oriimg = upImgs[0].clone();
        spdlog::info("oriimg size:{},{}", oriimg.cols, oriimg.rows);
        // cv::resize(upImgs[0], upImgs[0], cv::Size(ymlCameraCfg.outPutWidth, ymlCameraCfg.outPutHeight)); 
        
        spdlog::info("read takes:{} ms", sdkGetTimerValue(&timer));

        std::thread t1 = std::thread(&ocvStitcher::process, &ostitcherUp, std::ref(upImgs), std::ref(stitcherOut[0]));
        std::thread t2 = std::thread(&ocvStitcher::process, &ostitcherDown, std::ref(downImgs), std::ref(stitcherOut[1]));

        t1.join();
        t2.join();

        cv::resize(stitcherOut[0], stitcherOut[0], stitcherOut[1].size());

        cv::Mat up,down,ori;

        cv::vconcat(stitcherOut[0], stitcherOut[1], ret);
  
        // cv::imwrite("pano.png", ret);
        // cv::imwrite("ori.png", oriimg);

        // return 0;

        // ret = stitcherOut[0];

        spdlog::debug("ret size:[{},{}]", ret.size().width, ret.size().height);
        spdlog::info("stitching takes:{} ms", sdkGetTimerValue(&timer));

        if(start_ssr) 
            ret = nvProcessor->SSR(ret);

        if(detect)
        {
            ret = nvProcessor->ProcessOnce(ret);
            spdlog::debug("detect takes:{} ms", sdkGetTimerValue(&timer));
        }

        if(!writerInit && savevideo)
        {
            std::time_t tt = chrono::system_clock::to_time_t (chrono::system_clock::now());
            struct std::tm * ptm = std::localtime(&tt);
            stringstream sstr;
            sstr << std::put_time(ptm,"%F-%H-%M-%S");
            Size panoSize(ret.size().width, ret.size().height);
            Size oriSize(ori.size().width, ori.size().height);
            // panoWriter = new VideoWriter(sstr.str()+"-pano.avi", CV_FOURCC('I','4','2','0'), videoFps, panoSize);
           // panoWriter = new VideoWriter(sstr.str()+"-pano.avi", CV_FOURCC('I','4','2','0'), videoFps, Size(1920,1080));
             panoWriter = new VideoWriter(sstr.str()+"-pano.avi", CV_FOURCC('D','I','V','X'), videoFps, Size(1920,1080));
            // oriWriter = new VideoWriter(sstr.str()+"-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, oriSize);

            if (!panoWriter->isOpened())
            {
                spdlog::critical("Can not create video file.");
                return -1;
            }

            writerInit = true;
        }
        cv::Mat final;
        final = renderer->render(ret);
        // renderer->render(ret, final);
        // renderer->render(oriimg, final);

        // renderer->renderWithUi(ret, oriimg);
        nvProcessor->publishImage(final); 

        if(savevideo)
        {
            *panoWriter << final;
            // *oriWriter << ori;
        }
        
        spdlog::debug("frame [{}], render takes:{} ms", framecnt, sdkGetTimerValue(&timer));
        // setMouseCallback("ret",OnMouseAction);

        // if(detCamNum!=0)
        // {
        //     spdlog::critical("detCamNum::{}", detCamNum);
        //     cv::Mat croped = cameras[detCamNum-1]->m_distoredImg(cv::Rect(640, 300, 640, 480)).clone();
        //     croped = nvProcessor->ProcessOnce(croped);
        //     cv::imshow("det", croped);
        // }

        // char c = (char)cv::waitKey(1);
        // switch(c)
        // {
        //     case 27:
        //         return 0;
        //     case 'e':
        //         start_ssr = !start_ssr;
        //         break;
        //     case 'd':
        //         detect = !detect;
        //         break;
        //     case 'v':
        //         if(savevideo)
        //         {
        //             panoWriter->release();
        //             // oriWriter->release();
        //             writerInit = false;
        //         }
        //         savevideo = !savevideo;
        //         break;
        //     case 'o':
        //         displayori = !displayori;
        //         break;
        //     case 'x':
        //         detCamNum = 0;
        //         break;
        //     case 's':
        //         cv::imwrite("final.png", final);
        //         break;
        //     default:
        //         break;
        // }

        spdlog::info("frame [{}], all takes:{} ms", framecnt++, sdkGetTimerValue(&timer));
    }
    return 0;
}