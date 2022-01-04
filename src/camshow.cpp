#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>
#include "yaml-cpp/yaml.h"
#include "nvcam.hpp"
#include "PracticalSocket.h"
#include "stitcherconfig.h"
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
#endif

std::string cfgpath;
std::string defaultcfgpath = "../cfg/stitcher-imx390cfg.yaml";
int framecnt = 0;

bool detect = false;
bool showall = false;
bool withnum = false;
int idx = 3;
static int parse_cmdline(int argc, char **argv)
{
    int c;

    if (argc < 2)
    {
        return true;
    }

    while ((c = getopt(argc, argv, "c:dnp:")) != -1)
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
            default:
                break;
        }
    }
}

imageProcessor *nvProcessor = nullptr;

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    

    YAML::Node config = YAML::LoadFile(defaultcfgpath);
    camSrcWidth = config["camsrcwidth"].as<int>();
    camSrcHeight = config["camsrcheight"].as<int>();
    distorWidth = config["distorWidth"].as<int>();
    distorHeight = config["distorHeight"].as<int>();
    undistorWidth = config["undistorWidth"].as<int>();
    undistorHeight = config["undistorHeight"].as<int>();
    stitcherinputWidth = config["stitcherinputWidth"].as<int>();
    stitcherinputHeight = config["stitcherinputHeight"].as<int>();
    int USED_CAMERA_NUM = config["USED_CAMERA_NUM"].as<int>();
    std::string net = config["netpath"].as<string>();
    std::string cfgpath = config["camcfgpath"].as<string>();
    std::string canname = config["canname"].as<string>();
    showall = config["showall"].as<bool>();

    if(RET_ERR == parse_cmdline(argc, argv))
        return RET_ERR;

    imgs = std::vector<Mat>(CAMERA_NUM, Mat(stitcherinputHeight, stitcherinputWidth, CV_8UC4));
    
    if (detect)
        nvProcessor = new imageProcessor(net);  

    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,1,"/dev/video0"},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,2,"/dev/video1"},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,3,"/dev/video2"},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,4,"/dev/video3"},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,5,"/dev/video4"},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,6,"/dev/video5"},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,7,"/dev/video6"},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,8,"/dev/video7"}};

    std::shared_ptr<nvCam> cameras[CAMERA_NUM];
    if(showall)
    {
        for(int i=0;i<USED_CAMERA_NUM;i++)
            cameras[i].reset(new nvCam(camcfgs[i]));

        std::vector<std::thread> threads;
        for(int i=0;i<USED_CAMERA_NUM;i++)
            threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
        for(auto& th:threads)
            th.detach();
    }
    else
    {
        cameras[idx-1].reset(new nvCam(camcfgs[idx-1]));
        // std::thread t(&nvCam::run, cameras[idx-1].get());
        // t.detach();
    }



    Mat rets[USED_CAMERA_NUM];

    StopWatchInterface *timer = NULL;
    sdkCreateTimer(&timer);
    sdkResetTimer(&timer);
    sdkStartTimer(&timer);

    NvEglRenderer *nvrender  =  NvEglRenderer::createEglRenderer("renderer0", stitcherinputWidth, stitcherinputHeight, 0, 0);
    if(!nvrender)
        spdlog::critical("Failed to create EGL renderer");
    nvrender->setFPS(30);

    void *sBaseAddr[1] = {NULL};
    Mat mmat(1080, 1920, CV_8UC4);
    // Mat mmat = cv::Mat(1080, 1920, cv::CV_8UC4);
    NvBufferMemMap (cameras[idx-1]->ctx.render_dmabuf_fd, 0, NvBufferMem_Read_Write, &sBaseAddr[0]);
    mmat.data = (uchar*)sBaseAddr[0];
    
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
            std::thread server(serverCap);
            server.join();
#endif

            for(int i=0;i<USED_CAMERA_NUM;i++)
            {
                cameras[i]->getFrame(imgs[i]);
                if(withnum)
                    cv::putText(imgs[i], std::to_string(i+1), cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
            }
            nvrender->render(cameras[0]->ctx.render_dmabuf_fd);

            // cv::Mat up,down;
            // cv::hconcat(vector<cv::Mat>{upImgs[3], upImgs[2], upImgs[1], upImgs[0]}, up);
            // cv::hconcat(vector<cv::Mat>{downImgs[3], downImgs[2], downImgs[1], downImgs[0]}, down);
            // cv::vconcat(up, down, ret);

            // cv::Mat up,down;
            // cv::hconcat(vector<cv::Mat>{upImgs[2], upImgs[1], upImgs[0]}, up);
            // cv::hconcat(vector<cv::Mat>{upImgs[3], downImgs[1], downImgs[0]}, down);
            // cv::vconcat(up, down, ret);

            // cv::Mat up,down;
            // cv::hconcat(vector<cv::Mat>{upImgs[1], upImgs[0]}, up);
            // cv::hconcat(vector<cv::Mat>{upImgs[3], upImgs[2]}, down);
            // cv::vconcat(up, down, ret);

            // cv::Mat up,down;
            // cv::hconcat(vector<cv::Mat>{imgs[0], imgs[1], imgs[2], imgs[3]}, up);
            // cv::hconcat(vector<cv::Mat>{imgs[4], imgs[5], imgs[6], imgs[7]}, down);
            // cout<<"up type:"<<up.type()<<"down type:"<<down.type()<<endl;
            // cv::vconcat(up, down, ret);

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
            // cameras[idx-1]->getFrame(ret);
            cameras[idx-1]->read_frame();
            
            // ret = cameras[idx-1]->m_ret;

            // NvBufferMemSyncForCpu (cameras[idx-1]->ctx.render_dmabuf_fd, 0,&sBaseAddr[0]);
            // cv::putText(mmat, std::to_string(100), cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);

            // nvrender->render(cameras[idx-1]->ctx.render_dmabuf_fd);
            nvrender->render(cameras[idx-1]->retNvbuf[0].dmabuff_fd);
            // cv::imshow("mmm", mmat);
            // cv::waitKey(1);

            
#endif

            if(withnum)
                cv::putText(ret, std::to_string(idx), cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
        }

        spdlog::info("frame [{}], read takes:{} ms", framecnt, sdkGetTimerValue(&timer));
        
        // cv::Mat ori = ret.clone();
        cv::Mat yoloret = ret;
        if (detect)
        {
            yoloret = nvProcessor->ProcessOnce(ret);
        }

        // cv::imshow("m_dev_name", ret);
        char c = (char)cv::waitKey(1);
        switch(c)
        {
            case 's':
                if(showall)
                {
                    cv::imwrite("1.png", upImgs[0]);
                    cv::imwrite("2.png", upImgs[1]);
                    cv::imwrite("3.png", upImgs[2]);
                    cv::imwrite("4.png", upImgs[3]);
                    cv::imwrite("5.png", downImgs[0]);
                    cv::imwrite("6.png", downImgs[1]);
                    cv::imwrite("7.png", downImgs[2]);
                    cv::imwrite("8.png", downImgs[3]);
                }
                if (detect)
                {
                    cv::imwrite(std::to_string(idx) + "-" + std::to_string(framecnt)+".png", yoloret);
                }
                // cv::imwrite(std::to_string(idx) + "-ori" + std::to_string(framecnt++)+".png", ori);
                break;
            default:
                break;
        }

        spdlog::info("frame [{}], all takes:{} ms", framecnt++, sdkGetTimerValue(&timer));

    }
    return 0;
}