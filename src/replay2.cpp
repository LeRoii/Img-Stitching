#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>
#include "yaml-cpp/yaml.h"
#include "imageProcess.h"
#include "nvcam.hpp"
#include "PracticalSocket.h"
#include "ocvstitcher.hpp"
#include "helper_timer.h"
#include "nvrender.h"


// #define CAMERA_NUM 8
// #define USED_CAMERA_NUM 6
// #define BUF_LEN 65540 

using namespace cv;

static unsigned short servPort = 10001;
static UDPSocket sock(servPort);

static char buffer[SLAVE_PCIE_UDP_BUF_LEN]; // Buffer for echo string

vector<Mat> upImgs(4);
vector<Mat> downImgs(4);
vector<Mat> stitcherOut(2);
Mat upRet, downRet, ret;
int framecnt = 0;

#if CAM_IMX424
void serverCap()
{
    downImgs.clear();
    int recvMsgSize; // Size of received message
    string sourceAddress; // Address of datagram source
    unsigned short sourcePort; // Port of datagram source
    Mat recvedFrame;

    do {
        recvMsgSize = sock.recvFrom(buffer, SLAVE_PCIE_UDP_BUF_LEN, sourceAddress, sourcePort);
    } while (recvMsgSize > sizeof(int));
    int total_pack = ((int * ) buffer)[0];

    spdlog::info("expecting length of packs: {}", total_pack);
    char * longbuf = new char[SLAVE_PCIE_UDP_PACK_SIZE * total_pack];
    for (int i = 0; i < total_pack; i++) {
        recvMsgSize = sock.recvFrom(buffer, SLAVE_PCIE_UDP_BUF_LEN, sourceAddress, sourcePort);
        if (recvMsgSize != SLAVE_PCIE_UDP_PACK_SIZE) {
            spdlog::warn("Received unexpected size pack: {}", recvMsgSize);
            free(longbuf);
            return;
        }
        memcpy( & longbuf[i * SLAVE_PCIE_UDP_PACK_SIZE], buffer, SLAVE_PCIE_UDP_PACK_SIZE);
    }

    spdlog::debug("Received packet from {}:{}", sourceAddress, sourcePort);

    Mat rawData = Mat(1, SLAVE_PCIE_UDP_PACK_SIZE * total_pack, CV_8UC1, longbuf);
    recvedFrame = imdecode(rawData, IMREAD_COLOR);
    spdlog::debug("size:[{},{}]", recvedFrame.size().width, recvedFrame.size().height);
    if (recvedFrame.size().width == 0) {
        spdlog::warn("decode failure!");
        // continue;
    }
    downImgs[2] = recvedFrame(Rect(0,0,stitcherinputWidth, stitcherinputHeight)).clone();
    downImgs[3] = recvedFrame(Rect(stitcherinputWidth,0,stitcherinputWidth, stitcherinputHeight)).clone();
    // imwrite("7.png", downImgs[2]);
    // imwrite("8.png", downImgs[3]);
    // imshow("recv", recvedFrame);
    // waitKey(1);
    free(longbuf);
}

#endif

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
    YAML::Node config = YAML::LoadFile(stitchercfgpath);
    camSrcWidth = config["camsrcwidth"].as<int>();
    camSrcHeight = config["camsrcheight"].as<int>();
    distorWidth = config["distorWidth"].as<int>();
    distorHeight = config["distorHeight"].as<int>();
    undistorWidth = config["undistorWidth"].as<int>();
    undistorHeight = config["undistorHeight"].as<int>();
    stitcherinputWidth = config["stitcherinputWidth"].as<int>();
    stitcherinputHeight = config["stitcherinputHeight"].as<int>();
    num_images = config["num_images"].as<short int>();


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



    int finalcut = 15;
    if(stitcherinputWidth == 480)
        finalcut = 15;
    else if(stitcherinputWidth == 640)
        finalcut = 40;

    nvrenderCfg rendercfg{renderBufWidth, renderBufHeight, renderWidth, renderHeight, renderX, renderY, renderMode};
    nvrender *renderer = new nvrender(rendercfg);

    if(RET_ERR == parse_cmdline(argc, argv))
        return RET_ERR;

    if (detect)
        nvProcessor = new imageProcessor(net, canname, batchSize);  

    stStitcherCfg stitchercfg[2] = {stStitcherCfg{stitcherinputWidth, stitcherinputHeight, 1,num_images, stitcherMatchConf, stitcherAdjusterConf, stitcherBlenderStrength, stitcherCameraExThres, stitcherCameraInThres, cfgpath},
                                    stStitcherCfg{stitcherinputWidth, stitcherinputHeight, 2,num_images, stitcherMatchConf, stitcherAdjusterConf, stitcherBlenderStrength, stitcherCameraExThres, stitcherCameraInThres, cfgpath}};

    ocvStitcher ostitcherUp(stitchercfg[0]);
    ocvStitcher ostitcherDown(stitchercfg[1]);

    // upImgs.clear();
    // upImgs.push_back(imread("/home/nvidia/ssd/img/1.png"));
    // upImgs.push_back(imread("/home/nvidia/ssd/img/2.png"));
    // upImgs.push_back(imread("/home/nvidia/ssd/img/3.png"));
    // upImgs.push_back(imread("/home/nvidia/ssd/img/4.png"));

    // downImgs.clear();
    // downImgs.push_back(imread("/home/nvidia/ssd/img/5.png"));
    // downImgs.push_back(imread("/home/nvidia/ssd/img/6.png"));
    // downImgs.push_back(imread("/home/nvidia/ssd/img/7.png"));
    // downImgs.push_back(imread("/home/nvidia/ssd/img/8.png"));

    // upImgs.clear(); 
    // upImgs.push_back(imread("./1.png"));
    // upImgs.push_back(imread("./2.png"));
    // upImgs.push_back(imread("./3.png"));
    // upImgs.push_back(imread("./4.png"));

    // downImgs.clear();
    // downImgs.push_back(imread("./5.png"));
    // downImgs.push_back(imread("./6.png"));
    // downImgs.push_back(imread("./7.png"));
    // downImgs.push_back(imread("./8.png"));

    upImgs.clear(); 
    upImgs.push_back(imread("./1.png"));
    upImgs.push_back(imread("./2.png"));

    // upImgs.clear(); 
    // upImgs.push_back(imread("./5.png"));
    // upImgs.push_back(imread("./6.png"));
    // upImgs.push_back(imread("./7.png"));
    // upImgs.push_back(imread("./8.png"));

    // downImgs.clear();
    // downImgs.push_back(imread("./1.png"));
    // downImgs.push_back(imread("./2.png"));
    // downImgs.push_back(imread("./3.png"));
    // downImgs.push_back(imread("./4.png"));

    for(int i=0;i<2;i++)
    {
        cv::resize(upImgs[i], upImgs[i], cv::Size(stitcherinputWidth, stitcherinputHeight));
    }


    while(ostitcherUp.init(upImgs, initMode) != 0);
    spdlog::info("up init ok!!!!!!!!!!!!!!!!!!!!11 ");


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
        
        spdlog::info("read takes:{} ms", sdkGetTimerValue(&timer));

        /* parallel*/

        ostitcherUp.process(upImgs, ret);


        spdlog::debug("ret size:[{},{}]", ret.size().width, ret.size().height);

        spdlog::info("stitching takes:{} ms", sdkGetTimerValue(&timer));


        // if(ctl_command.use_ssr || start_ssr) 
        if(start_ssr)
        {
            ret = nvProcessor->SSR(ret);
            spdlog::debug("SSR takes:{} ms", sdkGetTimerValue(&timer));
        }

        if(detect)
        {
            // yoloRet = nvProcessor.Process(ret);

            std::vector<cv::Mat> imgs = {ret,ret,ret,ret};
            std::vector<std::vector<int>> dets;
            // nvProcessor->ImageDetect(imgs, dets); 
            ret = nvProcessor->ProcessOnce(ret);  
        //    if(ctl_command.use_detect || detect){
        //         nvProcessor.publishImage(yoloRet);
        //     } else{
                // nvProcessor.publishImage(ret);
            // }
            spdlog::debug("detect takes:{} ms", sdkGetTimerValue(&timer));
        }


        // cv::imshow("ret", ret);
        // cv::imshow("ret", final);
        
        spdlog::debug("render");
        Mat final;

        
#ifdef DEV_MODE
        renderer->render(ret, final);
#else
        renderer->render(ret);
#endif
        // setMouseCallback("ret",OnMouseAction);

        if(detCamNum!=0)
        {
            // spdlog::critical("detCamNum::{}", detCamNum);
            // cv::Mat croped = cameras[detCamNum-1]->m_distoredImg(cv::Rect(640, 300, 640, 480)).clone();
            // croped = nvProcessor->ProcessOnce(croped);
            // cv::imshow("det", croped);
        }

        if(saveret)
        {
            cv::imwrite("up.png", stitcherOut[0]);
            cv::imwrite("down.png", stitcherOut[1]);
        }

        char c = (char)cv::waitKey(1);
        switch(c)
        {
            case 27:
                return 0;
            case 'e':
                start_ssr = !start_ssr;
                break;
            case 'd':
                detect = !detect;
                break;
            case 'v':
                if(savevideo)
                {
                    panoWriter->release();
                    oriWriter->release();
                    writerInit = false;
                }
                savevideo = !savevideo;
                break;
            case 'o':
                displayori = !displayori;
                break;
            case 'x':
                detCamNum = 0;
                break;
            case 's':
                cv::imwrite("final.png", final);
                break;
            default:
                break;
        }

        spdlog::info("frame [{}], all takes:{} ms", framecnt++, sdkGetTimerValue(&timer));
    }
    return 0;
}