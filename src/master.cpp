#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>
#include "imageProcess.h"
#include "nvcam.hpp"
#include "PracticalSocket.h"
// #include "config.h"
#include "ocvstitcher.hpp"
#include "stitcherconfig.h"

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

std::mutex g_stitcherMtx[2];
std::condition_variable stitcherCon[2];
vector<vector<Mat>> stitcherInput{upImgs, downImgs};

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

    while ((c = getopt(argc, argv, "sdihv")) != -1)
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
            case 'h':
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


int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    parse_cmdline(argc, argv);

    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{3840,2160,1920/2,1080/2,stitcherinputWidth,stitcherinputHeight,1,"/dev/video0"},
                                    stCamCfg{3840,2160,1920/2,1080/2,stitcherinputWidth,stitcherinputHeight,2,"/dev/video1"},
                                    stCamCfg{3840,2160,1920/2,1080/2,stitcherinputWidth,stitcherinputHeight,3,"/dev/video2"},
                                    stCamCfg{3840,2160,1920/2,1080/2,stitcherinputWidth,stitcherinputHeight,4,"/dev/video3"},
                                    stCamCfg{3840,2160,1920/2,1080/2,stitcherinputWidth,stitcherinputHeight,5,"/dev/video4"},
                                    stCamCfg{3840,2160,1920/2,1080/2,stitcherinputWidth,stitcherinputHeight,6,"/dev/video5"},
                                    stCamCfg{3840,2160,1920/2,1080/2,stitcherinputWidth,stitcherinputHeight,7,"/dev/video6"}};

    std::shared_ptr<nvCam> cameras[USED_CAMERA_NUM];
    for(int i=0;i<USED_CAMERA_NUM;i++)
        cameras[i].reset(new nvCam(camcfgs[i]));

    // ocvStitcher ostitcherUp(960/2, 540/2, 1);
    // ocvStitcher ostitcherDown(960/2, 540/2, 2);
    ocvStitcher ostitcherUp(stitcherinputWidth, stitcherinputHeight, 1);
    ocvStitcher ostitcherDown(stitcherinputWidth, stitcherinputHeight, 2);

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

    // if(saveret)
    // {
    //     imwrite("1.png", upImgs[0]);
    //     imwrite("2.png", upImgs[1]);
    //     imwrite("3.png", upImgs[2]);
    //     imwrite("4.png", upImgs[3]);
    // }
    
    do{
        serverCap();
        cameras[4]->read_frame();
        cameras[5]->read_frame();
        downImgs[0] = cameras[4]->m_ret;
        downImgs[1] = cameras[5]->m_ret;
    }
    while(ostitcherDown.init(downImgs, initonline) != 0);
    spdlog::info("down init ok!!!!!!!!!!!!!!!!!!!!11 ");

    // if(saveret)
    // {
    //     imwrite("5.png", downImgs[0]);
    //     imwrite("6.png", downImgs[1]);
    //     imwrite("7.png", downImgs[2]);
    //     imwrite("8.png", downImgs[3]);
    // }

    // if(initonline)
    // {
    //     do{
    //         upImgs.clear();
    //         for(int i=0;i<4;i++)
    //         {
    //             cameras[i]->read_frame();
    //             upImgs.push_back(cameras[i]->m_ret);
    //         }   
    //     }
    //     while(ostitcherUp.initAll(upImgs) != 0);
    //     // while(ostitcherUp.simpleInit(upImgs) != 0);

    //     LOGLN("up init ok!!!!!!!!!!!!!!!!!!!!11 ");

    //     if(saveret)
    //     {
    //         imwrite("1.png", upImgs[0]);
    //         imwrite("2.png", upImgs[1]);
    //         imwrite("3.png", upImgs[2]);
    //         imwrite("4.png", upImgs[3]);
    //     }
        
    //     do{
    //         serverCap();
    //         cameras[4]->read_frame();
    //         cameras[5]->read_frame();
    //         downImgs[0] = cameras[4]->m_ret;
    //         downImgs[1] = cameras[5]->m_ret;
    //     }
    //     while(ostitcherDown.initAll(downImgs) != 0);
    //     // while(ostitcherDown.simpleInit(downImgs) != 0);

    //     LOGLN("down init ok!!!!!!!!!!!!!!!!!!!!11 ");

    //     if(saveret)
    //     {
    //         imwrite("5.png", downImgs[0]);
    //         imwrite("6.png", downImgs[1]);
    //         imwrite("7.png", downImgs[2]);
    //         imwrite("8.png", downImgs[3]);
    //     }

    // }
    // else
    // {
    //     upImgs.clear();
    //     Mat img = imread("/home/nvidia/ssd/code/0929IS/2222/1.png");
    //     // resize(img, img, Size(960/2, 540/2));
    //     upImgs.push_back(img);
    //     img = imread("/home/nvidia/ssd/code/0929IS/2222/2.png");
    //     // resize(img, img, Size(960/2, 540/2));
    //     upImgs.push_back(img);
    //     img = imread("/home/nvidia/ssd/code/0929IS/2222/3.png");
    //     // resize(img, img, Size(960/2, 540/2));
    //     upImgs.push_back(img);
    //     img = imread("/home/nvidia/ssd/code/0929IS/2222/4.png");
    //     // resize(img, img, Size(960/2, 540/2));
    //     upImgs.push_back(img);

    //     downImgs.clear();
    //     img = imread("/home/nvidia/ssd/code/0929IS/2222/5.png");
    //     // resize(img, img, Size(960/2, 540/2));
    //     downImgs.push_back(img);
    //     img = imread("/home/nvidia/ssd/code/0929IS/2222/6.png");
    //     // resize(img, img, Size(960/2, 540/2));
    //     downImgs.push_back(img);
    //     img = imread("/home/nvidia/ssd/code/0929IS/2222/7.png");
    //     // resize(img, img, Size(960/2, 540/2));
    //     downImgs.push_back(img);
    //     img = imread("/home/nvidia/ssd/code/0929IS/2222/8.png");
    //     // resize(img, img, Size(960/2, 540/2));
    //     downImgs.push_back(img);


    //     // while(ostitcherUp.init(upImgs) != 0){}
    //     // LOGLN("up init ok!!!!!!!!!!!!!!!!!!!!11 ");
    //     // while(ostitcherDown.init(downImgs) != 0){}
    //     // LOGLN("down init ok!!!!!!!!!!!!!!!!!!!!11 ");

    //     while(ostitcherUp.initSeam(upImgs) != 0){}
    //     LOGLN("up init ok!!!!!!!!!!!!!!!!!!!!11 ");
    //     while(ostitcherDown.initSeam(downImgs) != 0){}
    //     LOGLN("down init ok!!!!!!!!!!!!!!!!!!!!11 ");
    // }

    // return 0;

    std::vector<std::thread> threads;
    for(int i=0;i<USED_CAMERA_NUM;i++)
        threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
    for(auto& th:threads)
        th.detach();

    Mat rets[USED_CAMERA_NUM];

    imageProcessor nvProcessor;     //图像处理类

    std::thread st1 = std::thread(stitcherTh, 0, &ostitcherUp);
    std::thread st2 = std::thread(stitcherTh, 1, &ostitcherDown);

	//创建 writer，并指定 FOURCC 及 FPS 等参数
	VideoWriter *writer = nullptr;

    while(1)
    {
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
        
        std::thread server(serverCap);
        server.join();

        cameras[0]->getFrame(upImgs[0]);
        cameras[1]->getFrame(upImgs[1]);
        cameras[2]->getFrame(upImgs[2]);
        cameras[3]->getFrame(upImgs[3]);
        cameras[4]->getFrame(downImgs[0]);
        cameras[5]->getFrame(downImgs[1]);

        // for(int i=0;i<4;i++)
        //     imwrite(std::to_string(i+1)+".png", upImgs[i]);
        // for(int i=0;i<4;i++)
        //     imwrite(std::to_string(i+5)+".png", downImgs[i]);

        spdlog::debug("read takes : {:03.3f} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);
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

        cv::vconcat(upRet, downRet, ret);
        cv::rectangle(ret, cv::Rect(0, height - 2, width, 4), cv::Scalar(0,0,0), -1, 1, 0);

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

        if(detect)
        {
            controlData ctl_command;

            ctl_command = nvProcessor.getCtlCommand();
            spdlog::info("***********get command: ");
            spdlog::info("use_flip:{}, use_enh:{}, bright:{}, contrast:{}", ctl_command.use_flip, ctl_command.use_ssr, ctl_command.bright, ctl_command.contrast);

            cv::Mat yoloRet;
            // auto start = std::chrono::steady_clock::now();
            t = cv::getTickCount();
            
            if(ctl_command.use_ssr || start_ssr) {
                ret = nvProcessor.SSR(ret);
            }
            yoloRet = nvProcessor.Process(ret);
            
            // auto end = std::chrono::steady_clock::now();
            // std::chrono::duration<double> spent = end - start;
            // std::cout << " #############detect Time############: " << spent.count() << " sec \n";
            spdlog::debug("detect takes : {:03.3f} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);
           if(ctl_command.use_detect || detect){
                nvProcessor.publishImage(yoloRet);
            } else{
                nvProcessor.publishImage(ret);
            }
            
            cv::imshow("yolo", yoloRet);

            if(writer == nullptr)
            {
                Size s(yoloRet.size().width, yoloRet.size().height);
                writer = new VideoWriter("myvideod.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25, s);
                //检查是否成功创建
                if (!writer->isOpened())
                {
                    spdlog::critical("Can not create video file.");
                    return -1;
                }
            }
            if(savevideo)
                *writer << yoloRet;
            // cv::imshow("up", upRet);
            // cv::imshow("down", downRet);
            cv::waitKey(1);
        }
        else
        {
            if(writer == nullptr)
            {
                Size s(ret.size().width, ret.size().height);
                writer = new VideoWriter("myvideo.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25, s);
                //检查是否成功创建
                if (!writer->isOpened())
                {
                    spdlog::critical("Can not create video file.");
                    return -1;
                }
            }
            if(savevideo)
                *writer << ret;
            cv::imshow("ret", ret);
            // cv::imshow("up", stitcherOut[0]);
            // cv::imshow("down", stitcherOut[1]);
            // cv::imshow("ret", ret);
            cv::waitKey(1);
        }

        if(saveret)
        {
            cv::imwrite("up.png", stitcherOut[0]);
            cv::imwrite("down.png", stitcherOut[1]);
        }

        spdlog::debug("******all takes: {:03.3f} ms", ((getTickCount() - all) / getTickFrequency()) * 1000);

    }
    return 0;
}