#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>

#include "nvcam.hpp"
#include "PracticalSocket.h"
#include "config.h"
#include "ocvstitcher.hpp"


#define CAMERA_NUM 8
#define USED_CAMERA_NUM 6
#define BUF_LEN 65540 

#define LOG(msg) std::cout << msg
#define LOGLN(msg) std::cout << msg << std::endl

using namespace cv;

vector<Mat> upImgs(4);
vector<Mat> downImgs(4);
Mat upRet, downRet;

unsigned short servPort = 10001;
UDPSocket sock(servPort);
char buffer[BUF_LEN]; // Buffer for echo string


void serverCap()
{
    downImgs.clear();
    int recvMsgSize; // Size of received message
    string sourceAddress; // Address of datagram source
    unsigned short sourcePort; // Port of datagram source
    Mat recvedFrame;

    do {
        recvMsgSize = sock.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
    } while (recvMsgSize > sizeof(int));
    int total_pack = ((int * ) buffer)[0];

    cout << "expecting length of packs:" << total_pack << endl;
    char * longbuf = new char[PACK_SIZE * total_pack];
    for (int i = 0; i < total_pack; i++) {
        cout << "before recv msg" << endl;
        recvMsgSize = sock.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
        cout << "after recv msg recvMsgSize:" << recvMsgSize << endl;
        if (recvMsgSize != PACK_SIZE) {
            cerr << "Received unexpected size pack:" << recvMsgSize << endl;
            free(longbuf);
            return;
        }
        memcpy( & longbuf[i * PACK_SIZE], buffer, PACK_SIZE);
    }

    cout << "Received packet from " << sourceAddress << ":" << sourcePort << endl;

    Mat rawData = Mat(1, PACK_SIZE * total_pack, CV_8UC1, longbuf);
    recvedFrame = imdecode(rawData, IMREAD_COLOR);
    cout << "size:" << recvedFrame.size() << endl;
    if (recvedFrame.size().width == 0) {
        cerr << "decode failure!" << endl;
        // continue;
    }
    downImgs[2] = recvedFrame(Rect(0,0,480, 270)).clone();
    downImgs[3] = recvedFrame(Rect(480,0,480, 270)).clone();
    // imwrite("7.png", downImgs[2]);
    // imwrite("8.png", downImgs[3]);
    // imshow("recv", recvedFrame);
    // waitKey(1);
    free(longbuf);
}

int main()
{
    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,1,"/dev/video0"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,2,"/dev/video1"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,3,"/dev/video2"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,4,"/dev/video3"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,5,"/dev/video4"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,6,"/dev/video5"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,7,"/dev/video6"}};

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
        
        std::thread server(serverCap);
        server.join();

        // int ok = 1;
        // ok *= cameras[0]->getFrame(upImgs[0]);
        // ok *= cameras[1]->getFrame(upImgs[1]);
        // ok *= cameras[2]->getFrame(upImgs[2]);
        // ok *= cameras[3]->getFrame(upImgs[3]);
        // ok *= cameras[4]->getFrame(downImgs[0]);
        // ok *= cameras[5]->getFrame(downImgs[1]);

        // if(!ok)
        //     continue;

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

        // cv::Mat up,down, ret;
        // cv::hconcat(vector<cv::Mat>{cameras[0]->m_ret, cameras[1]->m_ret, cameras[2]->m_ret}, up);
        // cv::hconcat(vector<cv::Mat>{cameras[3]->m_ret, cameras[4]->m_ret, cameras[5]->m_ret}, down);
        // cv::vconcat(up, down, ret);
        // cv::imshow("m_dev_name", ret);

        // cv::imshow("1", upImgs[0]);
        cv::imshow("1", cameras[0]->m_ret);
        // cv::imwrite("1.png", cam0.m_ret);

        cv::waitKey(1);

        LOGLN("all takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");

    }
    return 0;
}