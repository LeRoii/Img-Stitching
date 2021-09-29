#include <opencv2/opencv.hpp>
#include <thread>
#include "stitcher.hpp"
#include <string>
#include "ocvstitcher.hpp"
#include "PracticalSocket.h"
#include "config.h"

#define USED_CAMERA_NUM 2
#define BUF_LEN 65540 

UDPSocket sock;
char buffer[BUF_LEN]; // Buffer for echo string

string servAddress = "192.168.44.100"; // First arg: server address
unsigned short servPort = Socket::resolveService("10001", "udp");

int jpegqual =  ENCODE_QUALITY;

int main(int argc, char *argv[])
{
    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,1,"/dev/video0"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,2,"/dev/video1"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,3,"/dev/video2"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,4,"/dev/video3"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,5,"/dev/video4"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,5,"/dev/video5"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/4,1080/4,6,"/dev/video6"}};
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

    // ocvStitcher ostitcher(960/2, 540/2);

    vector<Mat> imgs;
    Mat send;

    // do{
    //     imgs.clear();
    //     for(int i=0;i<USED_CAMERA_NUM;i++)
    //     {
    //         cameras[i].read_frame();
    //         imgs.push_back(cameras[i].m_ret);
    //     }   
    // }
    // while(ostitcher.init(imgs) != 0);
    
    // ostitcher.process(imgs, ret);

    // unsigned short servPort = 10000;
    // UDPSocket sock(servPort);
    // char buffer[BUF_LEN]; // Buffer for echo string
    // int recvMsgSize; // Size of received message
    // string sourceAddress; // Address of datagram source
    // unsigned short sourcePort; // Port of datagram source
    vector < uchar > encoded;
    Mat up,down;
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

        
        cv::hconcat(vector<Mat>{cameras[0].m_ret, cameras[1].m_ret}, up);

        //send to master
        vector < int > compression_params;
        compression_params.push_back(IMWRITE_JPEG_QUALITY);
        compression_params.push_back(jpegqual);

        send = up.clone();
        // send = imread("1.png");

        imencode(".jpg", send, encoded, compression_params);
        // imshow("send", send);
        int total_pack = 1 + (encoded.size() - 1) / PACK_SIZE;
        cout << "encodeed size:" << encoded.size() << "total_pack:" << total_pack << endl;

        int ibuf[1];
        ibuf[0] = total_pack;
        sock.sendTo(ibuf, sizeof(int), servAddress, servPort);
        cout << "before send data:" << endl;
        for (int i = 0; i < total_pack; i++)
            sock.sendTo( & encoded[i * PACK_SIZE], PACK_SIZE, servAddress, servPort);

        waitKey(1);
        

        // Mat rett;
        // vector<Mat> imgss(4);
        // imgss[0] = cameras[0].m_ret.clone();
        // imgss[1] = cameras[1].m_ret.clone();
        // imgss[2] = cameras[2].m_ret.clone();
        // imgss[3] = cameras[3].m_ret.clone();
        // ostitcher.process(imgss, rett);
            

        // std::thread th1(&gmslCamera::read_frame, std::ref(cameras[0]));
        // th1.join();
        // std::thread th2(&gmslCamera::read_frame, std::ref(cameras[1]));
        // th2.join();
        // imgstither.processnodet(cameras[1].m_ret, cameras[2].m_ret, ret);

		// printf("%p\n", cameras);
		// imgstither.processall(cameras, CAMERA_NUM, ret);
        
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
        else
        {
            // cv::imshow("m_dev_name", up);
            cv::waitKey(1);
        }
    }
    return 0;
}