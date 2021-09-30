#include <opencv2/opencv.hpp>
#include <thread>
#include "stitcher.hpp"
#include <string>
#include "ocvstitcher.hpp"
#include "PracticalSocket.h"
#include "imageProcess.h"
#include "config.h"

#define USED_CAMERA_NUM 6
#define BUF_LEN 65540 


unsigned short servPort = 10001;
UDPSocket sock(servPort);
char buffer[BUF_LEN]; // Buffer for echo string

vector<Mat> upImgs(4);
vector<Mat> downImgs(4);

void serverCap()
{
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
    imwrite("7.png", downImgs[2]);
    imwrite("8.png", downImgs[3]);
    // imshow("recv", recvedFrame);
    // waitKey(1);
    free(longbuf);
}

int main(int argc, char *argv[])
{
    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,1,"/dev/video0"},
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

    // stitcherCfg stitchercfg;
    // stitchercfg.imgHeight = camcfgs[0].retHeight;
    // stitchercfg.imgWidth = camcfgs[0].retWidth;

    // stitcher imgstither(stitchercfg, 1);

    // cv::Ptr<cv::Stitcher> stitcher = cv::Stitcher::create(cv::Stitcher::PANORAMA, true);

    ocvStitcher ostitcherUp(960/2, 540/2);
    ocvStitcher ostitcherDown(960/2, 540/2);

    // vector<Mat> imgs;
    
    do{
        upImgs.clear();
        for(int i=0;i<4;i++)
        {
            cameras[i].read_frame();
            upImgs.push_back(cameras[i].m_ret);
        }   
    }
    while(ostitcherUp.init(upImgs) != 0);

    LOGLN("up init ok!!!!!!!!!!!!!!!!!!!!11 ");
    

    do{
        serverCap();
        cameras[4].read_frame();
        cameras[5].read_frame();
        downImgs[0] = cameras[4].m_ret;
        downImgs[1] = cameras[5].m_ret;
    }
    while(ostitcherDown.init(downImgs) != 0);

    LOGLN("down init ok!!!!!!!!!!!!!!!!!!!!11 ");
    // do{
    //     imgs.clear();
    //     for(int i=0;i<4;i++)
    //     {
    //         cameras[i+4].read_frame();
    //         imgs.push_back(cameras[i].m_ret);
    //     }   
    // }
    // while(ostitcherUp.init(imgs) != 0);
    
    // ostitcherUp.process(imgs, ret);

    // unsigned short servPort = 10000;
    // UDPSocket sock(servPort);
    // char buffer[BUF_LEN]; // Buffer for echo string
    // int recvMsgSize; // Size of received message
    // string sourceAddress; // Address of datagram source
    // unsigned short sourcePort; // Port of datagram source

    imagePorcessor nvProcessor;

    while(1)
    {
        // cam0.read_frame();
        // cam1.read_frame();
        // cameras[0].read_frame();
		std::vector<std::thread> threads;
		for(int i=0;i<USED_CAMERA_NUM;i++)
			threads.push_back(std::thread(&gmslCamera::read_frame, std::ref(cameras[i])));
		for(auto& th:threads)
			th.join();
        
        std::thread server(serverCap);
        server.join();
        // int recvMsgSize; // Size of received message
        // string sourceAddress; // Address of datagram source
        // unsigned short sourcePort; // Port of datagram source

        // do {
        //     recvMsgSize = sock.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
        // } while (recvMsgSize > sizeof(int));
        // int total_pack = ((int * ) buffer)[0];

        // cout << "expecting length of packs:" << total_pack << endl;
        // char * longbuf = new char[PACK_SIZE * total_pack];
        // for (int i = 0; i < total_pack; i++) {
        //     cout << "before recv msg" << endl;
        //     recvMsgSize = sock.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
        //     cout << "after recv msg recvMsgSize:" << recvMsgSize << endl;
        //     if (recvMsgSize != PACK_SIZE) {
        //         cerr << "Received unexpected size pack:" << recvMsgSize << endl;
        //         continue;
        //     }
        //     memcpy( & longbuf[i * PACK_SIZE], buffer, PACK_SIZE);
        // }

        // cout << "Received packet from " << sourceAddress << ":" << sourcePort << endl;

        // Mat rawData = Mat(1, PACK_SIZE * total_pack, CV_8UC1, longbuf);
        // capedFrame = imdecode(rawData, IMREAD_COLOR);
        // cout << "size:" << capedFrame.size() << endl;
        // if (capedFrame.size().width == 0) {
        //     cerr << "decode failure!" << endl;
        //     continue;
        // }
        // // imwrite("rece.png", capedFrame);
        // imshow("recv", capedFrame);
        // // waitKey(1);
        // free(longbuf);

        Mat upRet, downRet;
        // vector<Mat> upImgs(4);
        // vector<Mat> downImgs(4);
        upImgs[0] = cameras[0].m_ret.clone();
        upImgs[1] = cameras[1].m_ret.clone();
        upImgs[2] = cameras[2].m_ret.clone();
        upImgs[3] = cameras[3].m_ret.clone();
        downImgs[0] = cameras[4].m_ret.clone();
        downImgs[1] = cameras[5].m_ret.clone();
        if(downImgs[2].empty() || downImgs[3].empty())
            continue;
        LOGLN("up process %%%%%%%%%%%%%%%%%%%");
        ostitcherUp.process(upImgs, upRet);
        LOGLN("down process %%%%%%%%%%%%%%%%%%%");
        ostitcherDown.process(downImgs, downRet);
            

        // Mat up,down;
        // cv::hconcat(vector<Mat>{cameras[0].m_ret, cameras[1].m_ret, cameras[2].m_ret}, up);
        // cv::hconcat(vector<Mat>{cameras[3].m_ret, cameras[4].m_ret, cameras[5].m_ret}, down);
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
        
        if(argc > 1)
        {
            cv::imwrite("final.png", ret);
            return 0;
        }
        else if(!upRet.empty())
        {
            cv::Mat yoloRet;
            yoloRet = nvProcessor.Process(upRet);
            // cv::imshow("yolo", yoloRet);
            // cv::imshow("up", upRet);
            // cv::imshow("down", downRet);
            cv::waitKey(1);
        }
    }
    return 0;
}