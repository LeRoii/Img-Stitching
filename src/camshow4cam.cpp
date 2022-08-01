#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>
#include "yaml-cpp/yaml.h"
#include "nvcam.hpp"
#include "PracticalSocket.h"
#include "nvrender.h"
#include "stitcherglobal.h"
#include "helper_timer.h"

using namespace cv;

vector<Mat> upImgs(4);
vector<Mat> downImgs(4);
Mat upRet, downRet;

vector<Mat> imgs(CAMERA_NUM);

std::string cfgpath;
std::string defaultcfgpath = "../cfg/stitcher-imx390cfg.yaml";

int framecnt = 0;

bool detect = false;
bool showall = false;
bool withnum = false;
int idx = 1;
int videoFps = 20;
bool savevideo = false;

static int parse_cmdline(int argc, char **argv)
{
    int c;

    if (argc < 2)
    {
        return true;
    }

    while ((c = getopt(argc, argv, "c:dnp:v")) != -1)
    {
        switch (c)
        {
            case 'c':
                if (strcmp(optarg, "a") == 0)
                {
                    showall = true;
                }
                else
                {
                    if(strlen(optarg) == 1 && std::isdigit(optarg[0]))
                    {
                        showall = false;
                        idx = std::stoi(optarg);
//#ifdef CAM_IMX390
                        stitcherinputWidth = undistorWidth = 1920;
                        stitcherinputHeight = undistorHeight = 1080;
//#endif
                        if(0 < idx < 9)
                            break;
                    }
                    spdlog::critical("invalid argument!!!\n");
                    return RET_ERR;
                }
                break;
            case 'p':
                cfgpath = optarg;
                spdlog::info("cfg path:{}", cfgpath);
                if(std::string::npos == cfgpath.find(".yaml"))
                    spdlog::warn("input cfgpath invalid, use default");
                else
                    defaultcfgpath = cfgpath;
                break;
            case 'd':
                detect = true;
                break;
            case 'n':
                withnum = true;
                break;
            case 'v':
                savevideo = true;
                spdlog::warn("savevideo");
                break;
            default:
                break;
        }
    }
}


int main(int argc, char *argv[])
{
    YAML::Node config = YAML::LoadFile(defaultcfgpath);
    spdlog::info("defaultcfgpath:{}", defaultcfgpath);
    stitcherinputWidth = config["stitcherinputWidth"].as<int>();
    stitcherinputHeight = config["stitcherinputHeight"].as<int>();

    // if(RET_ERR == parse_cmdline(argc, argv))
    //     return RET_ERR;
    
    vendor = config["vendor"].as<int>();
    camSrcWidth = config["camsrcwidth"].as<int>();
    camSrcHeight = config["camsrcheight"].as<int>();
    distorWidth = config["distorWidth"].as<int>();
    distorHeight = config["distorHeight"].as<int>();
    undistorWidth = config["undistorWidth"].as<int>();
    undistorHeight = config["undistorHeight"].as<int>();
    
    renderWidth = config["renderWidth"].as<int>();
    renderHeight = config["renderHeight"].as<int>();
    renderX = config["renderX"].as<int>();
    renderY = config["renderY"].as<int>();
    renderBufWidth = config["renderBufWidth"].as<int>();
    renderBufHeight = config["renderBufHeight"].as<int>();

    int USED_CAMERA_NUM = config["USED_CAMERA_NUM"].as<int>();
    std::string net = config["netpath"].as<string>();
    std::string cfgpath = config["camcfgpath"].as<string>();
    std::string canname = config["canname"].as<string>();
    // showall = config["showall"].as<bool>();
    undistor = config["undistor"].as<bool>();
    renderMode = config["renderMode"].as<int>();

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
        
        stitcherinputWidth = undistorWidth = 1920/2;
        stitcherinputHeight = undistorHeight = 1080/2;
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

    nvrenderCfg rendercfg{renderBufWidth, renderBufHeight, renderWidth, renderHeight, renderX, renderY, renderMode};
    nvrender *renderer = new nvrender(rendercfg);


    imgs = std::vector<Mat>(CAMERA_NUM, Mat(stitcherinputHeight, stitcherinputWidth, CV_8UC4));
    

    VideoWriter *writer[CAMERA_NUM];
    if(savevideo)
    {
        for(int i=0;i<USED_CAMERA_NUM;i++)
            writer[i] = new VideoWriter(std::to_string(i)+"-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, Size(1920,1080));
    }

    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,1,"/dev/video0", vendor},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,2,"/dev/video1", vendor},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,3,"/dev/video2", vendor},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,4,"/dev/video3", vendor},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,5,"/dev/video4", vendor},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,6,"/dev/video5", vendor},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,7,"/dev/video6", vendor},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,8,"/dev/video7", vendor}};

    std::shared_ptr<nvCam> cameras[CAMERA_NUM];
    // if(showall)
    // {
    //     for(int i=0;i<USED_CAMERA_NUM;i++)
    //         cameras[i].reset(new nvCam(camcfgs[i]));

    //     std::vector<std::thread> threads;
    //     for(int i=0;i<USED_CAMERA_NUM;i++)
    //         threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
    //     for(auto& th:threads)
    //         th.detach();
    // }
    // else
    // {
    //     cameras[idx-1].reset(new nvCam(camcfgs[idx-1]));
    //     // std::thread t(&nvCam::run, cameras[idx-1].get());
    //     // t.detach();
    // }

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

    // NvEglRenderer *nvrender  =  NvEglRenderer::createEglRenderer("renderer0", stitcherinputWidth/2, stitcherinputHeight/2, 0, 0);
    // if(!nvrender)
    //     spdlog::critical("Failed to create EGL renderer");
    // nvrender->setFPS(30);

    // Mat mmat(1080, 1920, CV_8UC4);
    // Mat mmat = cv::Mat(1080, 1920, cv::CV_8UC4);
    // NvBufferMemMap (cameras[idx-1]->ctx.render_dmabuf_fd, 0, NvBufferMem_Read_Write, &sBaseAddr[0]);
    // NvBufferMemMap (cameras[idx-1]->retNvbuf->dmabuff_fd, 0, NvBufferMem_Read_Write, (void**)&mmat.data);
    // mmat.data = (uchar*)sBaseAddr[0];

    // cv::VideoCapture capture("/home/nvidia/ssd/code/Img-Stitching/build/2021-11-19-16-44-28-pano.avi");
    // spdlog::warn("is open:{}", capture.isOpened());

    while(1)
    {
        sdkResetTimer(&timer);
        
        cv::Mat ret;

        if(showall)
        {
            for(int i=0;i<USED_CAMERA_NUM;i++)
            {
                // cameras[i]->read_frame();
                cameras[i]->getFrame(imgs[i], false);
                if(withnum)
                    cv::putText(imgs[i], std::to_string(i+1), cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
            }

            if(savevideo)
            {
                for(int i=0;i<USED_CAMERA_NUM;i++)
                    *writer[i] << imgs[i];
            }
            else
            {          
                cv::Mat up,down;
                // spdlog::info("imgs[4] channel:{}, imgs[6]:{}", imgs[4].channels(), imgs[6].channels());
                cv::hconcat(vector<cv::Mat>{imgs[0], imgs[1]}, up);
                cv::hconcat(vector<cv::Mat>{imgs[2], imgs[3]}, down);
                cv::vconcat(up, down, ret);
            }

        }
        else
        {
            cameras[idx-1]->getFrame(ret);
            // cv::imshow("1", ret);
            // cv::imwrite("1.png", ret);
            // cv::waitKey(1000);
            // cameras[idx-1]->read_frame();
            
            // ret = cameras[idx-1]->m_ret;

            // NvBufferMemSyncForCpu (cameras[idx-1]->ctx.render_dmabuf_fd, 0,&sBaseAddr[0]);
            // cv::putText(mmat, std::to_string(100), cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);

            // nvrender->render(cameras[idx-1]->ctx.render_dmabuf_fd);
            // nvrender->render(cameras[idx-1]->retNvbuf->dmabuff_fd);
            // cv::imshow("mmm", cameras[idx-1]->m_ret);
            // cv::waitKey(1);
            
            if(withnum)
                cv::putText(ret, std::to_string(idx), cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
        }

        spdlog::info("frame [{}], read takes:{} ms", framecnt, sdkGetTimerValue(&timer));
        
        // cv::Mat ori = ret.clone();

        // capture >> ret;
        // cv::cvtColor(ret,ret,cv::COLOR_RGB2RGBA);
        
        
        cv::Mat yoloret;// = ret.clone();
        // cv::Mat yoloret = ret(cv::Rect(640, 300, 640, 480)).clone();

        if(!savevideo)
        {
            spdlog::info("render");
            renderer->render(ret);
            // if(stitcherinputWidth==1920 && showall)
            //     renderer->render(imgs[1]);
            // else
            //     renderer->render(ret);
                // cv::imshow("m_dev_name", ret);
            // else
            //     cv::imshow("m_dev_name", ret);
            //*writer[0] << ret;
        }

        
        char c = (char)cv::waitKey(1);
        switch(c)
        {
            case 's':
                if(showall)
                {
                    for(int i=0;i<USED_CAMERA_NUM;i++)
                        cv::imwrite(std::to_string(i)+".png", imgs[i]);
                }
                if (detect)
                {
                    cv::imwrite(std::to_string(idx) + "-" + std::to_string(framecnt)+".png", yoloret);
                }
                cv::imwrite(std::to_string(idx) + "-ori" + std::to_string(framecnt++)+".png", ret);
                break;
            default:
                break;
        }
        spdlog::info("frame [{}], all takes:{} ms", framecnt++, sdkGetTimerValue(&timer));

    }
    return 0;
}