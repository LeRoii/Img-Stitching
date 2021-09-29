#include <opencv2/opencv.hpp>
#include <thread>
#include "stitcher.hpp"
#include <string>
#include "ocvstitcher.hpp"
#include "PracticalSocket.h"
#include "config.h"

#define USED_CAMERA_NUM 1
#define BUF_LEN 65540 

unsigned short servPort = 10001;
UDPSocket sock(servPort);
char buffer[BUF_LEN]; // Buffer for echo string

vector<Mat> upImgs(4);
vector<Mat> downImgs(4);

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
    if (rawData.size().width == 0) {
        cerr << "decode failure!" << endl;
        return;
    }
    recvedFrame = imdecode(rawData, IMREAD_COLOR);
    cout << "size:" << recvedFrame.size() << endl;
   
    downImgs[2] = recvedFrame(Rect(0,0,480, 270)).clone();
    downImgs[3] = recvedFrame(Rect(480,0,480, 270)).clone();
    imwrite("7.png", downImgs[2]);
    imwrite("8.png", downImgs[3]);
    // imshow("recv", recvedFrame);
    // waitKey(1);
    free(longbuf);
}

int main(int argc, char *argv[])
{
    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{3840,2160,1920/2,1080/2,1920/2,1080/2,1,"/dev/video0"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,2,"/dev/video1"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,3,"/dev/video2"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,4,"/dev/video3"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,5,"/dev/video4"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,6,"/dev/video5"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,7,"/dev/video6"}};
    gmslCamera cameras[CAMERA_NUM] = {gmslCamera{camcfgs[0]},
                                    gmslCamera{camcfgs[1]},
                                    gmslCamera{camcfgs[2]},
                                    gmslCamera{camcfgs[3]},
                                    gmslCamera{camcfgs[4]},
                                    gmslCamera{camcfgs[5]},
                                    gmslCamera{camcfgs[6]}};
    cv::Mat ret;// = cv::Mat(2160, 3840, CV_8UC3);

    
    // std::vector<std::thread> threads;
    // for(int i=0;i<USED_CAMERA_NUM;i++)
    //     threads.push_back(std::thread(&gmslCamera::run, std::ref(cameras[i])));
    // for(auto& th:threads)
    //     th.detach();

    while(1)
    {
        auto t = getTickCount();

        cameras[0].read_frame();
        cv::imshow("1", cameras[0].m_ret);
        cv::waitKey(1);
        LOGLN("get imgs takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");

        // cameras[0].getFrame(upImgs[0]);
        // cameras[1].getFrame(upImgs[1]);
        // cameras[2].getFrame(upImgs[2]);
        // cameras[3].getFrame(upImgs[3]);
        // cameras[4].getFrame(downImgs[0]);
        // cameras[5].getFrame(downImgs[1]);

        // for(int i=0;i<USED_CAMERA_NUM;i++)
        //     cameras[i].read_frame();
        // int test = 1;
        // test *= upImgs[0].size().width;
        // test *= upImgs[1].size().width;
        // test *= upImgs[2].size().width;
        // test *= upImgs[3].size().width;
        // test *= downImgs[0].size().width;
        // test *= downImgs[1].size().width;
        // if(test == 0)
        //     continue;
        // for(int i=0;i<USED_CAMERA_NUM;i++)
        // {    
        //     test *= upImgs[i].size().width()
        //     if(upImgs[i].size().width == 0)
        //         continue;
        //     if(downImgs[0].size().width == 0 || downImgs[1].size().width == 0)
        //         continue;
        // }
        // LOGLN("get imgs takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");
        
        // std::thread server(serverCap);
        // server.join();

        // Mat upRet, downRet;
        // vector<Mat> upImgs(4);
        // vector<Mat> downImgs(4);
        // upImgs[0] = cameras[0].m_ret.clone();
        // upImgs[1] = cameras[1].m_ret.clone();
        // upImgs[2] = cameras[2].m_ret.clone();
        // upImgs[3] = cameras[3].m_ret.clone();
        // downImgs[0] = cameras[4].m_ret.clone();
        // downImgs[1] = cameras[5].m_ret.clone();

        // Mat up,down;
        // cv::hconcat(vector<Mat>{upImgs[0], upImgs[1], upImgs[2], upImgs[3]}, up);
        // cv::hconcat(vector<Mat>{downImgs[0], downImgs[1], downImgs[2], downImgs[3]}, down);
        // cv::hconcat(vector<Mat>{downImgs[0], downImgs[1]}, down);
        // cv::vconcat(up, down, ret);
        // cv::imshow("m_dev_name", cameras[0].m_ret);
        // ret = cameras[0].m_ret;

        // imgstither.stitcherProcess(cameras, USED_CAMERA_NUM, ret);

        // std::vector<cv::Mat> imgs;
        // for(int i=0;i<USED_CAMERA_NUM;i++)
        //     imgs.push_back(cameras[i].m_ret);

        // // cv::Stitcher cvStitcher = cv::Stitcher::createDefault(false);
        // cv::Stitcher::Status status = stitcher->stitch(imgs, ret);

        // if (status != cv::Stitcher::OK) 
        // {   
        //     std::cout << "Can't stitch images, error code = " << int(status) << std::endl;   
        //     return -1;   
        // }   
        
        // if(argc > 1)
        // {
        //     cv::imwrite("final.png", ret);
        //     return 0;
        // }
        // else
        // {
        //     // cv::imshow("up", upRet);
        //     // cv::imshow("down", downRet);
        //     // cv::imshow("ret", ret);
        //     // cv::imshow("1", cameras[0].m_ret);
        //     // cv::imshow("up", up);
        //     // cv::imshow("down", down);
                // cv::imshow("1", upImgs[0]);
        //     // cv::imshow("3", upImgs[2]);
            // LOGLN("all takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");

            cv::waitKey(1);
        // }

        
    }
    return 0;
}

