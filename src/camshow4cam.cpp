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

static vector<Mat> imgs(4);
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

bool showall = false;
int idx = 1;

#ifdef DEV_MODE
static std::string stitchercfgpath = "../cfg/stitcher-imx390cfg.yaml";
#else
static std::string stitchercfgpath = "/etc/panorama/stitcher-imx390cfg.yaml";
#endif

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
    while ((c = getopt(argc, argv, "sdev")) != -1)
    {
        switch (c)
        {
            case 's':
                saveret = true;
                break;
            case 'd':
                detect = true;
                break;
            case 'e':
                start_ssr = true;
                break;
            case  'v':
                savevideo = true;
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

        auto camDispIdx = config["camDispIdx"].as<string>();

        if("a" == camDispIdx)
        {
            showall = true;
        }
        else
        {
            try{
                idx = std::stoi(camDispIdx);
            }
            catch(std::invalid_argument&){
                spdlog::warn("invalid camDispIdx");
                idx = 1;
            }
            
            // ymlCameraCfg.outPutWidth = ymlCameraCfg.undistoredWidth = 1920;
            // ymlCameraCfg.outPutHeight = ymlCameraCfg.undistoredHeight = 1080;
        }

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
    }
    catch(...)
    {
        spdlog::critical("yml parse failed, check your config yaml!");
        return RET_ERR;
    }

    return RET_OK;
}

// int scanKeyboard()
// {
//   //  struct termios
//   //    {
//   //      tcflag_t c_iflag;		/* input mode flags */
//   //      tcflag_t c_oflag;		/* output mode flags */
//   //      tcflag_t c_cflag;		/* control mode flags */
//   //      tcflag_t c_lflag;		/* local mode flags */
//   //      cc_t c_line;			/* line discipline */
//   //      cc_t c_cc[NCCS];		/* control characters */
//   //      speed_t c_ispeed;		/* input speed */
//   //      speed_t c_ospeed;		/* output speed */
//   //  #define _HAVE_STRUCT_TERMIOS_C_ISPEED 1
//   //  #define _HAVE_STRUCT_TERMIOS_C_OSPEED 1
//   //    };
//   while(1 && g_keyboardinput!=113)
//   {
    
//     struct termios new_settings;
//     struct termios stored_settings;
//     tcgetattr(STDIN_FILENO,&stored_settings); //获得stdin 输入
//     new_settings = stored_settings;           //
//     new_settings.c_lflag &= (~ICANON);        //
//     new_settings.c_cc[VTIME] = 0;
//     tcgetattr(STDIN_FILENO,&stored_settings); //获得stdin 输入
//     new_settings.c_cc[VMIN] = 1;
//     tcsetattr(STDIN_FILENO,TCSANOW,&new_settings); //

//     g_keyboardinput = getchar();

//     tcsetattr(STDIN_FILENO,TCSANOW,&stored_settings);

//     // setbit(g_usrcmd, SETTING_ON);
//     // if(g_keyboardinput == 101)  //e
//     //     setbit(g_usrcmd, SETTING_IMGENHANCE);
//     // else if(g_keyboardinput == 100) //d
//     //     reversebit(g_usrcmd, SETTING_DETECTION);
//     // else if(g_keyboardinput == 99)  //c
//     //     reversebit(g_usrcmd, SETTING_CROSS);
//     // else if(g_keyboardinput == 115)  //s
//     //     reversebit(g_usrcmd, SETTING_SAVE);
//     // return g_keyboardinput;
//   }
// }

imageProcessor *nvProcessor = nullptr;
jetsonEncoder *encoder = nullptr;

int main(int argc, char *argv[])
{
    if(RET_ERR == parse_cmdline(argc, argv))
        return RET_ERR;

    if(RET_ERR == parseYml())
        return RET_ERR;

    nvrenderCfg rendercfg{renderBufWidth, renderBufHeight, renderWidth, renderHeight, renderX, renderY, renderMode};
    nvrenderAlpha *renderer = new nvrenderAlpha(rendercfg);

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

	// VideoWriter *panoWriter = nullptr;
	// VideoWriter *oriWriter = nullptr;
    // bool writerInit = false;

    StopWatchInterface *timer = NULL;
    sdkCreateTimer(&timer);
    sdkResetTimer(&timer);
    sdkStartTimer(&timer);

    // std::thread listerner = std::thread(scanKeyboard);
    // listerner.detach();
    
    while(1)
    {
        spdlog::debug("start loop");
        sdkResetTimer(&timer);
        
        if(showall)
        {
            cameras[0]->getFrame(imgs[0], false);
            cameras[1]->getFrame(imgs[1], false);
            cameras[2]->getFrame(imgs[2], false);
            cameras[3]->getFrame(imgs[3], false);

            cv::Mat up,down;
            cv::hconcat(vector<cv::Mat>{imgs[0], imgs[1]}, up);
            cv::hconcat(vector<cv::Mat>{imgs[2], imgs[3]}, down);
            cv::vconcat(up, down, ret);
        }
        else
        {
            cameras[idx-1]->getFrame(ret);
        }

        spdlog::info("read takes:{} ms", sdkGetTimerValue(&timer));
  
        spdlog::debug("ret size:[{},{}]", ret.size().width, ret.size().height);
        spdlog::info("concat takes:{} ms", sdkGetTimerValue(&timer));


        // // if(detect)
        // // {
        //     ret = nvProcessor->ProcessOnce(ret);
        // //     spdlog::debug("detect takes:{} ms", sdkGetTimerValue(&timer));
        // // }

        // if(!writerInit && savevideo)
        // {
        //      std::time_t tt = chrono::system_clock::to_time_t (chrono::system_clock::now());
        //      struct std::tm * ptm = std::localtime(&tt);
        //      stringstream sstr;
        //      sstr << std::put_time(ptm,"%F-%H-%M-%S");
        //      Size panoSize(ret.size().width, ret.size().height);
        //      Size oriSize(ori.size().width, ori.size().height);
        //      // panoWriter = new VideoWriter(sstr.str()+"-pano.avi", CV_FOURCC('I','4','2','0'), videoFps, panoSize);
        //     // panoWriter = new VideoWriter(sstr.str()+"-pano.avi", CV_FOURCC('I','4','2','0'), videoFps, Size(1920,1080));
        //      panoWriter = new VideoWriter(sstr.str()+"-pano.avi", CV_FOURCC('D','I','V','X'), videoFps, Size(1280,720));
        //      // oriWriter = new VideoWriter(sstr.str()+"-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, oriSize);

        //      if (!panoWriter->isOpened())
        //      {
        //          spdlog::critical("Can not create video file.");
        //          return -1;
        //      }

        //      writerInit = true;
        //  }
        cv::Mat final;
        final = renderer->render(ret);
        // renderer->render(ret, final);
        // renderer->render(oriimg, final);

        // renderer->renderWithUi(ret, oriimg);
        if(websocketOn)
            // encoder->process(final); 
            encoder->sendBase64(final); 

        // if(savevideo)
        // {
        //     *panoWriter << final;
        //     // *oriWriter << ori;
        // }

        // cv::imwrite("final.png", stitcherOut[1]); 
        // return 0;

        // cv::imshow("1", stitcherOut[1]);
        char c = (char)cv::waitKey(1);
        if(c=='s')
        {
            cv::imwrite("1.png", imgs[0]);
            cv::imwrite("2.png", imgs[1]);
        }
        
        spdlog::debug("frame [{}], render takes:{} ms", framecnt, sdkGetTimerValue(&timer));

        spdlog::info("frame [{}], all takes:{} ms", framecnt++, sdkGetTimerValue(&timer));

        // if(g_keyboardinput == 'v')
        // {
        //     if(savevideo)
        //     {
        //         spdlog::critical("panoWriter is opened:{}",panoWriter->isOpened());
        //         spdlog::critical("g_keyboardinput:{}",g_keyboardinput);
        //         panoWriter->release();
        //         savevideo = false;
        //         spdlog::critical("panoWriter is opened:{}",panoWriter->isOpened());
        //         return 0;
        //     }
        //     else
        //     {
        //         savevideo = true;
        //     }

        //     g_keyboardinput = 0;
        // }
    }
    return 0;
}