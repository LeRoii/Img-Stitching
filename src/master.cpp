#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>
#include "yaml-cpp/yaml.h"
#include "imageProcess.h"
#include "nvcam.hpp"
#include "PracticalSocket.h"
// #include "config.h"
#include "ocvstitcher.hpp"
#include "helper_timer.h"


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

bool saveret = false;
bool detect = false;
bool initonline = false;
bool start_ssr = false;
bool savevideo = false;
bool displayori = false;
int videoFps = 10;

std::mutex g_stitcherMtx[2];
std::condition_variable stitcherCon[2];
vector<vector<Mat>> stitcherInput{upImgs, downImgs};
std::string stitchercfgpath = "/home/nvidia/ssd/code/1221/cfg/stitcher-imx390cfg.yaml";

void stitcherTh(int id, ocvStitcher *stitcher)
{
    while(1)
    {
        std::unique_lock<std::mutex> lock(g_stitcherMtx[id]);
        while(!stitcher->inputOk)
            stitcherCon[id].wait(lock);
        stitcher->process(stitcherInput[id], stitcherOut[id]);
        // stitcher->simpprocess(stitcherInput[id], stitcherOut[id]);
        stitcher->inputOk = false;
        stitcher->outputOk = true;
        stitcherCon[id].notify_all();
    }
}

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

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    parse_cmdline(argc, argv);

    YAML::Node config = YAML::LoadFile(stitchercfgpath);
    camSrcWidth = config["camsrcwidth"].as<int>();
    camSrcHeight = config["camsrcheight"].as<int>();
    undistorWidth = config["undistorWidth"].as<int>();
    undistorHeight = config["undistorHeight"].as<int>();
    stitcherinputWidth = config["stitcherinputWidth"].as<int>();
    stitcherinputHeight = config["stitcherinputHeight"].as<int>();
    USED_CAMERA_NUM = config["USED_CAMERA_NUM"].as<int>();
    std::string net = config["netpath"].as<string>();
    std::string cfgpath = config["camcfgpath"].as<string>();
    std::string canname = config["canname"].as<string>();

    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,1,"/dev/video0"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,2,"/dev/video1"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,3,"/dev/video2"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,4,"/dev/video3"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,5,"/dev/video4"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,6,"/dev/video5"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,7,"/dev/video6"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,8,"/dev/video7"}};


    static std::shared_ptr<nvCam> cameras[CAMERA_NUM];
    for(int i=0;i<USED_CAMERA_NUM;i++)
        cameras[i].reset(new nvCam(camcfgs[i]));

    /************************************stitch all *****************************************/
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
    /************************************stitch all end*****************************************/

    ocvStitcher ostitcherUp(stitcherinputWidth, stitcherinputHeight, 1, cfgpath);
    ocvStitcher ostitcherDown(stitcherinputWidth, stitcherinputHeight, 2, cfgpath);

    do{
        upImgs.clear();
        for(int i=0;i<4;i++)
        {
            cameras[i]->read_frame();
            upImgs.push_back(cameras[i]->m_ret);
        }   
    }
    while(ostitcherUp.init(upImgs, initonline) != 0);
    spdlog::info("up init ok!!!!!!!!!!!!!!!!!!!!11 ");


    do{
#if CAM_IMX390
        downImgs.clear();
        for(int i=0;i<4;i++)
        {
            cameras[i+4]->read_frame();
            downImgs.push_back(cameras[i]->m_ret);
        }
#elif CAM_IMX424
        serverCap();
        cameras[4]->read_frame();
        cameras[5]->read_frame();
        downImgs[0] = cameras[4]->m_ret;
        downImgs[1] = cameras[5]->m_ret;
#endif
    }
    while(ostitcherDown.init(downImgs, initonline) != 0);


    spdlog::info("down init ok!!!!!!!!!!!!!!!!!!!!11 ");


    std::vector<std::thread> threads;
    for(int i=0;i<USED_CAMERA_NUM;i++)
        threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
    for(auto& th:threads)
        th.detach();

    imageProcessor nvProcessor(net);     //图像处理类

    std::thread st1 = std::thread(stitcherTh, 0, &ostitcherUp);
    std::thread st2 = std::thread(stitcherTh, 1, &ostitcherDown);

	VideoWriter *panoWriter = nullptr;
	VideoWriter *oriWriter = nullptr;
    bool writerInit = false;

    
    while(1)
    {
        spdlog::debug("start loop");
        auto t = cv::getTickCount();
        auto all = cv::getTickCount();
        // cameras[0]->read_frame();
        // cameras[1]->read_frame();
        // cameras[2]->read_frame();
        // cameras[3]->read_frame();
        // cameras[4]->read_frame();
        // cameras[5]->read_frame();

        /*slow */
        // std::vector<std::thread> threads;
        // for(int i=0;i<USED_CAMERA_NUM;i++)
        //     threads.push_back(std::thread(&nvCam::read_frame, cameras[i].get()));
        // for(auto& th:threads)
        //     th.join();
        
#if CAM_IMX424
        spdlog::debug("capture slave start");
        std::thread server(serverCap);
#endif
        
        cameras[0]->getFrame(upImgs[0]);
        cameras[1]->getFrame(upImgs[1]);
        cameras[2]->getFrame(upImgs[2]);
        cameras[3]->getFrame(upImgs[3]);
        cameras[4]->getFrame(downImgs[0]);
        cameras[5]->getFrame(downImgs[1]);
#if CAM_IMX390
        cameras[6]->getFrame(downImgs[2]);
        cameras[7]->getFrame(downImgs[3]);
#endif
        spdlog::debug("master cap fini");
#if CAM_IMX424
        server.join();
        spdlog::debug("slave cap fini");
#endif
        

        // for(int i=0;i<4;i++)
        //     imwrite(std::to_string(i+1)+".png", upImgs[i]);
        // for(int i=0;i<4;i++)
        //     imwrite(std::to_string(i+5)+".png", downImgs[i]);

        spdlog::info("read takes : {:03.3f} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);
        t = cv::getTickCount();

        // cv::imshow("ret", upImgs[2]);
        // cv::imshow("ret", cameras[2]->m_ret);
        // cv::waitKey(1);
        // continue;

        /* serial execute*/
        // LOGLN("up process %%%%%%%%%%%%%%%%%%%");
        // ostitcherUp.process(upImgs, upRet);
        // LOGLN("down process %%%%%%%%%%%%%%%%%%%");
        // ostitcherDown.process(downImgs, downRet);
        
        // upRet = upRet(Rect(0,20,1185,200));
        // downRet = downRet(Rect(0,25,1185,200));
        
        /* parallel*/
        stitcherInput[0][0] = upImgs[0];
        stitcherInput[0][1] = upImgs[1];
        stitcherInput[0][2] = upImgs[2];
        stitcherInput[0][3] = upImgs[3];

        stitcherInput[1][0] = downImgs[0];
        stitcherInput[1][1] = downImgs[1];
        stitcherInput[1][2] = downImgs[2];
        stitcherInput[1][3] = downImgs[3];

        std::unique_lock<std::mutex> lock(g_stitcherMtx[0]);
        std::unique_lock<std::mutex> lock1(g_stitcherMtx[1]);

        ostitcherUp.inputOk = true;
        ostitcherDown.inputOk = true;

        stitcherCon[0].notify_all();
        stitcherCon[1].notify_all();
        
        while(!(ostitcherUp.outputOk && ostitcherDown.outputOk))
        {
            stitcherCon[0].wait(lock);
            stitcherCon[1].wait(lock1);
        }

        ostitcherUp.outputOk = false;
        ostitcherDown.outputOk = false;

        int width = min(stitcherOut[0].size().width, stitcherOut[1].size().width);
        int height = min(stitcherOut[0].size().height, stitcherOut[1].size().height) - 30;
        upRet = stitcherOut[0](Rect(0,15,width,height));
        downRet = stitcherOut[1](Rect(0,15,width,height));

        cv::Mat up,down,ori;
        if(displayori)
        {
            cv::hconcat(vector<cv::Mat>{upImgs[3], upImgs[2], upImgs[1], upImgs[0]}, up);
            cv::hconcat(vector<cv::Mat>{downImgs[3], downImgs[2], downImgs[1], downImgs[0]}, down);
            cv::vconcat(up, down, ori);
        }

        cv::vconcat(upRet, downRet, ret);
        cv::rectangle(ret, cv::Rect(0, height - 2, width, 4), cv::Scalar(0,0,0), -1, 1, 0);

        spdlog::debug("ret size:[{},{}]", ret.size().width, ret.size().height);

        // cv::Mat up,down, ret;
        // cv::hconcat(vector<cv::Mat>{cameras[0]->m_ret, cameras[1]->m_ret, cameras[2]->m_ret}, up);
        // cv::hconcat(vector<cv::Mat>{cameras[3]->m_ret, cameras[4]->m_ret, cameras[5]->m_ret}, down);
        // cv::vconcat(up, down, ret);
        // cv::imshow("m_dev_name", ret);

        // cv::imshow("1", cam0.m_ret);
        // cv::imwrite("1.png", cam0.m_ret);

        // if(saveret)
        // {
        //     imwrite("1.png", upImgs[0]);
        //     imwrite("2.png", upImgs[1]);
        //     imwrite("3.png", upImgs[2]);
        //     imwrite("4.png", upImgs[3]);
        //     imwrite("5.png", downImgs[0]);
        //     imwrite("6.png", downImgs[1]);
        //     imwrite("7.png", downImgs[2]);
        //     imwrite("8.png", downImgs[3]);
        // }

        controlData ctl_command;
        ctl_command = nvProcessor.getCtlCommand();
        spdlog::info("***********get command: ");
        spdlog::info("use_flip:{}, use_enh:{}, bright:{}, contrast:{}", ctl_command.use_flip, ctl_command.use_ssr, ctl_command.bright, ctl_command.contrast);

        // if(ctl_command.use_ssr || start_ssr) 
        if(start_ssr) 
            ret = nvProcessor.SSR(ret);

        if(detect)
        {
            t = cv::getTickCount();
            // yoloRet = nvProcessor.Process(ret);
            ret = nvProcessor.ProcessOnce(ret);
            spdlog::info("detect takes : {:03.3f} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);
        //    if(ctl_command.use_detect || detect){
        //         nvProcessor.publishImage(yoloRet);
        //     } else{
                // nvProcessor.publishImage(ret);
            // }
        }

        if(!writerInit && savevideo)
        {
            std::time_t tt = chrono::system_clock::to_time_t (chrono::system_clock::now());
            struct std::tm * ptm = std::localtime(&tt);
            stringstream sstr;
            sstr << std::put_time(ptm,"%F-%H-%M-%S");
            Size panoSize(ret.size().width, ret.size().height);
            Size oriSize(ori.size().width, ori.size().height);
            panoWriter = new VideoWriter(sstr.str()+"-pano.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, panoSize);
            oriWriter = new VideoWriter(sstr.str()+"-ori.avi", CV_FOURCC('M', 'J', 'P', 'G'), videoFps, oriSize);

            //检查是否成功创建
            if (!panoWriter->isOpened() || !oriWriter->isOpened())
            {
                spdlog::critical("Can not create video file.");
                return -1;
            }

            writerInit = true;
        }
        if(savevideo)
        {
            *panoWriter << ret;
            *oriWriter << ori;
        }

        cv::imshow("ret", ret);
        setMouseCallback("ret",OnMouseAction);

        if(detCamNum!=0)
        {
            spdlog::critical("detCamNum::{}", detCamNum);
            cv::Mat croped = cameras[detCamNum-1]->m_distoredImg(cv::Rect(640, 300, 640, 480)).clone();
            croped = nvProcessor.ProcessOnce(croped);
            cv::imshow("det", croped);
        }

        if(displayori)
            cv::imshow("ori", ori);

        
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
            default:
                break;
        }

        spdlog::info("******all takes: {:03.3f} ms", ((getTickCount() - all) / getTickFrequency()) * 1000);

    }
    return 0;
}