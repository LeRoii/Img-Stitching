#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>

#include "nvcam.hpp"
#include "PracticalSocket.h"
#include "ocvstitcher.hpp"
#include "stitcherconfig.h"
#include "imageProcess.h"

// #define CAMERA_NUM 8
// #define USED_CAMERA_NUM 6
// #define BUF_LEN 65540 

#define LOG(msg) std::cout << msg
#define LOGLN(msg) std::cout << msg << std::endl

using namespace cv;

vector<Mat> upImgs(4);
vector<Mat> downImgs(4);
Mat upRet, downRet;

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
    downImgs[2] = recvedFrame(Rect(0,0,stitcherinputWidth, stitcherinputHeight)).clone();
    downImgs[3] = recvedFrame(Rect(stitcherinputWidth,0,stitcherinputWidth, stitcherinputHeight)).clone();
    // imwrite("7.png", downImgs[2]);
    // imwrite("8.png", downImgs[3]);
    // imshow("recv", recvedFrame);
    // waitKey(1);
    free(longbuf);
}

std::string command;
int framecnt = 0;

void ketboardlistener()
{
    while(1)
    {
        std::cin >> command;
        std::cout << "command:" << command << std::endl;
    }
}


bool detect = false;
bool showall = false;
int idx = 1;
static int 
parse_cmdline(int argc, char **argv)
{
    int c;

    if (argc < 2)
    {
        return true;
    }

    while ((c = getopt(argc, argv, "c:d")) != -1)
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
                        idx = std::stoi(optarg);
                        if(0 < idx < 9)
                            break;
                    }
                    spdlog::critical("invalid argument!!!\n");
                    return RET_ERR;
                }
                break;
            case 'd':
                detect = true;
                break;
            default:
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    // if(argc > 1)
    // {
    //     if(argv[1][0] <'1' || argv[1][0] > '8')
    //     {
    //         spdlog::critical("invalid argument!!!\n");
    //         return 0;
    //     }
    // }

    if(RET_ERR == parse_cmdline(argc, argv))
        return RET_ERR;

    thread kblistener(ketboardlistener);

    imageProcessor nvProcessor;  

    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,1,"/dev/video0"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,2,"/dev/video1"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,3,"/dev/video2"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,4,"/dev/video3"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,5,"/dev/video4"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,6,"/dev/video5"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,7,"/dev/video6"},
                                    stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,8,"/dev/video7"}};

    std::shared_ptr<nvCam> cameras[USED_CAMERA_NUM];
    for(int i=0;i<USED_CAMERA_NUM;i++)
        cameras[i].reset(new nvCam(camcfgs[i], true));

    std::vector<std::thread> threads;
    for(int i=0;i<USED_CAMERA_NUM;i++)
        threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
    for(auto& th:threads)
        th.detach();

    Mat rets[USED_CAMERA_NUM];

    while(1)
    {
        auto t = cv::getTickCount();
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
            std::thread server(serverCap);
            server.join();

            cameras[0]->getFrame(upImgs[0]);
            cameras[1]->getFrame(upImgs[1]);
            cameras[2]->getFrame(upImgs[2]);
            cameras[3]->getFrame(upImgs[3]);
            cameras[4]->getFrame(downImgs[0]);
            cameras[5]->getFrame(downImgs[1]);

            cv::putText(upImgs[0], "1", cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
            cv::putText(upImgs[1], "2", cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
            cv::putText(upImgs[2], "3", cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
            cv::putText(upImgs[3], "4", cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
            cv::putText(downImgs[0], "5", cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
            cv::putText(downImgs[1], "6", cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
            cv::putText(downImgs[2], "7", cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
            cv::putText(downImgs[3], "8", cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);


            cv::Mat up,down;
            cv::hconcat(vector<cv::Mat>{upImgs[3], upImgs[2], upImgs[1], upImgs[0]}, up);
            cv::hconcat(vector<cv::Mat>{downImgs[3], downImgs[2], downImgs[1], downImgs[0]}, down);
            cv::vconcat(up, down, ret);

        }
        else
        {
            // int idx = stoi(argv[1]);
            if(idx < 5)
            {
                cameras[idx-1]->getFrame(ret);
                cv::putText(ret, std::to_string(idx), cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);
            }
            else
            {
                std::thread server(serverCap);
                cameras[4]->getFrame(downImgs[0]);
                cameras[5]->getFrame(downImgs[1]);
                server.join();
                ret = downImgs[idx-5];
                cv::putText(ret, std::to_string(idx), cv::Point(20, 20), cv::FONT_HERSHEY_COMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, 8, 0);

            }
        }

        
        // for(int i=0;i<4;i++)
        //     imwrite(std::to_string(i+1)+".png", upImgs[i]);
        // for(int i=0;i<4;i++)
        //     imwrite(std::to_string(i+5)+".png", downImgs[i]);

        LOGLN("read takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");
        t = cv::getTickCount();

        cv::Mat yoloret;
        if (detect)
        {
            ret = nvProcessor.ProcessOnce(ret);
        }

        if(command == "s")
        {
            cv::imwrite(std::to_string(framecnt++)+".png", ret);
            command = "";
        }

        cv::imshow("m_dev_name", ret);
        char c = (char)cv::waitKey(1);
        switch(c)
        {
            case 's':
            cv::imwrite("1.png", upImgs[0]);
            cv::imwrite("2.png", upImgs[1]);
            cv::imwrite("3.png", upImgs[2]);
            cv::imwrite("4.png", upImgs[3]);
            cv::imwrite("5.png", downImgs[0]);
            cv::imwrite("6.png", downImgs[1]);
            cv::imwrite("7.png", downImgs[2]);
            cv::imwrite("8.png", downImgs[3]);
            break;
            default:
            break;
        }

        LOGLN("all takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");

    }
    return 0;
}