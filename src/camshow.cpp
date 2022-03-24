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

#if CAM_IMX424
unsigned short servPort = 10001;
UDPSocket sock(servPort);
char buffer[SLAVE_PCIE_UDP_BUF_LEN]; // Buffer for echo string

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

    spdlog::debug("expecting length of packs: {}", total_pack);
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
    // downImgs[2] = recvedFrame(Rect(0,0,stitcherinputWidth, stitcherinputHeight)).clone();
    // downImgs[3] = recvedFrame(Rect(stitcherinputWidth,0,stitcherinputWidth, stitcherinputHeight)).clone();
    imgs[6] = recvedFrame(Rect(0,0,stitcherinputWidth, stitcherinputHeight)).clone();
    imgs[7] = recvedFrame(Rect(stitcherinputWidth,0,stitcherinputWidth, stitcherinputHeight)).clone();
    // imwrite("7.png", downImgs[2]);
    // imwrite("8.png", downImgs[3]);
    // imshow("recv", recvedFrame);
    // waitKey(1);
    free(longbuf);
}

void serverCap2()
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
    // downImgs[2] = recvedFrame(Rect(0,0,stitcherinputWidth, stitcherinputHeight)).clone();
    // downImgs[3] = recvedFrame(Rect(stitcherinputWidth,0,stitcherinputWidth, stitcherinputHeight)).clone();
    imgs[7] = recvedFrame.clone();
    // imwrite("7.png", downImgs[2]);
    // imwrite("8.png", downImgs[3]);
    // imshow("recv", recvedFrame);
    // waitKey(1);
    free(longbuf);
}
#endif

std::string cfgpath;
std::string defaultcfgpath = "../cfg/stitcher-imx390cfg.yaml";
int framecnt = 0;

bool detect = false;
bool showall = true;
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
#ifdef CAM_IMX390
                        stitcherinputWidth = undistorWidth = camSrcWidth;
                        stitcherinputHeight = undistorHeight = camSrcHeight;
                        
#endif
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

imageProcessor *nvProcessor = nullptr;

int main(int argc, char *argv[])
{
    YAML::Node config = YAML::LoadFile(defaultcfgpath);
    stitcherinputWidth = config["stitcherinputWidth"].as<int>();
    stitcherinputHeight = config["stitcherinputHeight"].as<int>();


    if(RET_ERR == parse_cmdline(argc, argv))
        return RET_ERR;

    
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
    
    if (detect)
        nvProcessor = new imageProcessor(net);

    VideoWriter *writer[8];
    if(savevideo)
    {
        writer[0] = new VideoWriter("0-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, Size(1920,1080));
        writer[1] = new VideoWriter("1-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, Size(1920,1080));
        writer[2] = new VideoWriter("2-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, Size(1920,1080));
        writer[3] = new VideoWriter("3-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, Size(1920,1080));
        writer[4] = new VideoWriter("4-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, Size(1920,1080));
        writer[5] = new VideoWriter("5-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, Size(1920,1080));
        writer[6] = new VideoWriter("6-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, Size(1920,1080));
        writer[7] = new VideoWriter("7-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, Size(1920,1080));
    }

    // writer[0] = new VideoWriter("0-ori.mp4", CV_FOURCC('m', 'p', '4', 'v'), videoFps, Size(1920,1080));
    //writer[0] = new VideoWriter("0-ori.avi", CV_FOURCC('I', '4', '2', '0'), videoFps, Size(1920,1080));


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
        // cameras[0]->read_frame();
        // cameras[1]->read_frame();
        // cameras[2]->read_frame();
        // cameras[3]->read_frame();
        // cameras[4]->read_frame();
        // cameras[5]->read_frame();

        // return 0;

        /*slow */
        // std::vector<std::thread> threads;
        // for(int i=0;i<USED_CAMERA_NUM;i++)
        //     threads.push_back(std::thread(&nvCam::read_frame, cameras[i].get()));
        // for(auto& th:threads)
        //     th.join();
        
        cv::Mat ret;

        if(showall)
        {
#if CAM_IMX424
            spdlog::info("wait for slave");
            std::thread server(serverCap2);
            server.join();
#endif
            spdlog::info("wait for slave end");
            for(int i=0;i<USED_CAMERA_NUM;i++)
            {
                // cameras[i]->read_frame();
                cameras[i]->getFrame(imgs[i]);
                if(withnum)
                    cv::putText(imgs[i], std::to_string(i+1), cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
            }

            if(savevideo)
            {
                for(int i=0;i<8;i++)
                    *writer[i] << imgs[i];
            }
            else
            {          
                cv::Mat up,down;
                cv::hconcat(vector<cv::Mat>{imgs[0], imgs[1], imgs[2], imgs[3]}, up);
                cv::hconcat(vector<cv::Mat>{imgs[4], imgs[5], imgs[6], imgs[7]}, down);
                cv::vconcat(up, down, ret);
            }

        }
        else
        {
#if CAM_IMX424
            if(idx < 5)
            {
                cameras[idx-1]->getFrame(ret);
            }
            else
            {
                std::thread server(serverCap);
                cameras[4]->getFrame(downImgs[0]);
                cameras[5]->getFrame(downImgs[1]);
                server.join();
                ret = downImgs[idx-5];
            }
#elif CAM_IMX390
            cameras[idx-1]->getFrame(ret);
            
            // cameras[idx-1]->read_frame();
            
            // ret = cameras[idx-1]->m_ret;

            // NvBufferMemSyncForCpu (cameras[idx-1]->ctx.render_dmabuf_fd, 0,&sBaseAddr[0]);
            // cv::putText(mmat, std::to_string(100), cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);

            // nvrender->render(cameras[idx-1]->ctx.render_dmabuf_fd);
            // nvrender->render(cameras[idx-1]->retNvbuf->dmabuff_fd);
            // cv::imshow("mmm", cameras[idx-1]->m_ret);
            // cv::waitKey(1);
            
#endif

            if(withnum)
                cv::putText(ret, std::to_string(idx), cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
        }

        spdlog::info("frame [{}], read takes:{} ms", framecnt, sdkGetTimerValue(&timer));
        
        // cv::Mat ori = ret.clone();

        // capture >> ret;
        // cv::cvtColor(ret,ret,cv::COLOR_RGB2RGBA);
        
        
        cv::Mat yoloret;// = ret.clone();
        // cv::Mat yoloret = ret(cv::Rect(640, 300, 640, 480)).clone();
        if (detect)
        {
            yoloret = nvProcessor->ProcessOnce(ret);
        }

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
                    cv::imwrite("1.png", imgs[0]);
                    cv::imwrite("2.png", imgs[1]);
                    cv::imwrite("3.png", imgs[2]);
                    cv::imwrite("4.png", imgs[3]);
                    cv::imwrite("5.png", imgs[4]);
                    cv::imwrite("6.png", imgs[5]);
                    cv::imwrite("7.png", imgs[6]);
                    cv::imwrite("8.png", imgs[7]);
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