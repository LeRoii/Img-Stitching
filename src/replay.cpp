#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>
#include "yaml-cpp/yaml.h"
#include "imageProcess.h"
#include "nvcam.hpp"
#include "PracticalSocket.h"
#include "ocvstitcher.hpp"
#include "helper_timer.h"
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
int stitcherinputWidth=480;

stCamCfg ymlCameraCfg;
static bool websocketOn;
static int websocketPort;



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
        
    }
}

static int parseYml()
{
    try
    {
        YAML::Node config = YAML::LoadFile(stitchercfgpath);
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

        num_images = config["num_images"].as<int>();

        renderWidth = config["renderWidth"].as<int>();
        renderHeight = config["renderHeight"].as<int>();
        renderX = config["renderX"].as<int>();
        renderY = config["renderY"].as<int>();
        renderBufWidth = config["renderBufWidth"].as<int>();
        renderBufHeight = config["renderBufHeight"].as<int>();

        USED_CAMERA_NUM = config["USED_CAMERA_NUM"].as<int>();

        renderMode = config["renderMode"].as<int>();

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

    stNvrenderCfg rendercfg{renderBufWidth, renderBufHeight, renderWidth, renderHeight, renderX, renderY, renderMode};
    nvrenderAlpha *renderer = new nvrenderAlpha(rendercfg);

    nvProcessor = new imageProcessor();
    nvProcessor->init(stitchercfgpath);
    encoder = new jetsonEncoder(websocketOn, websocketPort);

    ocvStitcher ostitcherUp;
    ocvStitcher ostitcherDown;

    int failnum = 0;

    upImgs[0] = cv::imread("/home/nvidia/ssd/data/4cam/2/0.png");
    upImgs[1] = cv::imread("/home/nvidia/ssd/data/4cam/2/1.png");

    downImgs[0] = cv::imread("/home/nvidia/ssd/data/4cam/2/2.png");
    downImgs[1] = cv::imread("/home/nvidia/ssd/data/4cam/2/3.png");

    cv::resize(upImgs[0], upImgs[0], cv::Size(640, 360));
    cv::resize(upImgs[1], upImgs[1], cv::Size(640, 360));
    cv::resize(downImgs[0], downImgs[0], cv::Size(640, 360));
    cv::resize(downImgs[1], downImgs[1], cv::Size(640, 360));
    
    if(RET_ERR == ostitcherUp.init(stitchercfgpath))
    {
        spdlog::critical("stitcher init failed, check yml parameters");
        return RET_ERR;
    }
    if(RET_ERR == ostitcherDown.init(stitchercfgpath))
    {
        spdlog::critical("stitcher init failed, check yml parameters");
        return RET_ERR;
    }

    if(RET_ERR == ostitcherUp.calibration(upImgs))
    {
        spdlog::critical("stitcher calibration failed");
        return RET_ERR;
    }
    if(RET_ERR == ostitcherDown.calibration(downImgs))
    {
        spdlog::critical("stitcher calibration failed");
        return RET_ERR;
    }


	VideoWriter *panoWriter = nullptr;
	VideoWriter *oriWriter = nullptr;
    cv::Mat ori;
    
    bool writerInit = false;

    StopWatchInterface *timer = NULL;
    sdkCreateTimer(&timer);
    sdkResetTimer(&timer);
    sdkStartTimer(&timer);
    int ys=0;
    
    while(1)
    {
        spdlog::debug("start loop");
        sdkResetTimer(&timer);
        
        // cameras[0]->getFrame(upImgs[0], false);
        // cameras[1]->getFrame(upImgs[1], false);
        // cameras[2]->getFrame(upImgs[2], false);
        // cameras[3]->getFrame(upImgs[3], false);
        // cameras[4]->getFrame(downImgs[0], false);
        // cameras[5]->getFrame(downImgs[1], false);
        // cameras[6]->getFrame(downImgs[2], false);
        // cameras[7]->getFrame(downImgs[3], false);
        
        spdlog::info("read takes:{} ms", sdkGetTimerValue(&timer));

        /* serial execute*/
        // LOGLN("up process %%%%%%%%%%%%%%%%%%%");
        // ostitcherUp.process(upImgs, stitcherOut[0]);
        // LOGLN("down process %%%%%%%%%%%%%%%%%%%");
        // ostitcherDown.process(downImgs, stitcherOut[1]);
        
        // upRet = upRet(Rect(0,20,1185,200));
        // downRet = downRet(Rect(0,25,1185,200));
        
        /* parallel*/

        std::thread t1 = std::thread(&ocvStitcher::process, &ostitcherUp, std::ref(upImgs), std::ref(stitcherOut[0]));
        std::thread t2 = std::thread(&ocvStitcher::process, &ostitcherDown, std::ref(downImgs), std::ref(stitcherOut[1]));

        t1.join();
        t2.join();


        cv::resize(stitcherOut[0], stitcherOut[0], stitcherOut[1].size());
        cv::vconcat(stitcherOut[0], stitcherOut[1], ret);
        //cv::rectangle(ret, cv::Rect(0, ret.rows/2-5, ret.cols, 10), cv::Scalar(0,0,0), -1);

        spdlog::debug("ret size:[{},{}]", ret.size().width, ret.size().height);
        spdlog::info("stitching takes:{} ms", sdkGetTimerValue(&timer));

        if(start_ssr) 
            ret = nvProcessor->SSR(ret);

        if(detect)
        {
            ret = nvProcessor->ProcessOnce(ret);
            spdlog::debug("detect takes:{} ms", sdkGetTimerValue(&timer));
        }
        // spdlog::info("ret size:[{},{}]", ret.size().width, ret.size().height);
        // if(ys==0){
        // cv::Mat dst,dst1,dst2,dst3;
        // cv::resize(ret,dst,cv::Size(900,int(ret.size().height*900/ret.size().width)));
        // cv::resize(ret,dst1,cv::Size(800,int(ret.size().height*800/ret.size().width)));
        // cv::resize(ret,dst2,cv::Size(700,int(ret.size().height*700/ret.size().width)));
        // cv::resize(ret,dst3,cv::Size(600,int(ret.size().height*600/ret.size().width)));
        // cv::imwrite("ret.png",ret);
        // cv::imwrite("900.png",dst);
        // cv::imwrite("800.png",dst1);
        // cv::imwrite("700.png",dst2);
        // cv::imwrite("600.png",dst3);
        // ys++;
        // }

        if(!writerInit && savevideo)
        {
            std::time_t tt = chrono::system_clock::to_time_t (chrono::system_clock::now());
            struct std::tm * ptm = std::localtime(&tt);
            stringstream sstr;
            sstr << std::put_time(ptm,"%F-%H-%M-%S");
            Size panoSize(ret.size().width, ret.size().height);
            Size oriSize(ori.size().width, ori.size().height);
            // panoWriter = new VideoWriter(sstr.str()+"-pano.avi", CV_FOURCC('I','4','2','0'), videoFps, panoSize);
            panoWriter = new VideoWriter(sstr.str()+"-pano.avi", CV_FOURCC('I','4','2','0'), videoFps, Size(1920,1080));
            // oriWriter = new VideoWriter(sstr.str()+"-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, oriSize);

            //检查是否成功创建
            // if (!panoWriter->isOpened() || !oriWriter->isOpened())
            // {
            //     spdlog::critical("Can not create video file.");
            //     return -1;
            // }
            if (!panoWriter->isOpened())
            {
                spdlog::critical("Can not create video file.");
                return -1;
            }

            writerInit = true;
        }
        cv::Mat final;
        // spdlog::info("ret size:[{},{}]", ret.size().width, ret.size().height);
        final = renderer->render(ret);
        // spdlog::info("final size:[{},{}]", final.size().width, final.size().height);

        
        if(savevideo)
        {
            *panoWriter << final;
            // *oriWriter << ori;
        }
        
        spdlog::debug("frame [{}], render takes:{} ms", framecnt, sdkGetTimerValue(&timer));
        // setMouseCallback("ret",OnMouseAction);

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
                    // oriWriter->release();
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