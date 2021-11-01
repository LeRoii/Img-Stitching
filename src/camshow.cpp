#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>

#include "nvcam.hpp"
#include "PracticalSocket.h"
#include "ocvstitcher.hpp"
#include "stitcherconfig.h"


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

int main(int argc, char *argv[])
{
    if(argc > 1)
    {
        printf("aaaaaaaaaaaaaa::::%c\n", argv[1][0]);
        if(argv[1][0] <'0' || argv[1][0] > '8')
        {
            printf("invalid argument!!!\n");
            return 0;
        }
    }

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
        
        if(argc == 1)
        {
            std::thread server(serverCap);
            server.join();
        }

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

        LOGLN("read takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");
        t = cv::getTickCount();


        // cameras[0]->read_frame();
        // // cameras[1]->read_frame();
        // if(!rets[0].empty())
        //     cv::imshow("1", rets[0]);
        // cv::imshow("1", cameras[0]->m_ret);
        // cv::imshow("2", cameras[1]->m_ret);
        // cv::imshow("3", cameras[2]->m_ret);
        // cv::imshow("4", cameras[3]->m_ret);
        // cv::waitKey(1);

        cv::Mat ret;
        if(argc == 1)
        {
            cv::Mat up,down;
            cv::hconcat(vector<cv::Mat>{upImgs[3], upImgs[2], upImgs[1], upImgs[0]}, up);
            cv::hconcat(vector<cv::Mat>{downImgs[3], downImgs[2], downImgs[1], downImgs[0]}, down);
            cv::vconcat(up, down, ret);
            
        }
        else
        {
            int idx = stoi(argv[1]);
            
            if(idx < 5)
            {
                ret = upImgs[idx-1];
            }
            else
            {
                ret = downImgs[idx-5];
            }
        }

        cv::imshow("m_dev_name", ret);

        // cv::imshow("1", upImgs[0]);
        // cv::imshow("5", cameras[0]->m_ret);
        // cv::imwrite("1.png", cam0.m_ret);

        cv::waitKey(1);

        LOGLN("all takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");

    }
    return 0;
}