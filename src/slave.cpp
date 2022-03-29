// #include <opencv2/opencv.hpp>
// #include <thread>
// #include <string>
// // #include "ocvstitcher.hpp"
// #include "PracticalSocket.h"
// #include "nvcam.hpp"
// #include "stitcherglobal.h"

// #define USED_CAMERA_NUM 1
// #define CAMERA_NUM 2
// #define BUF_LEN 65540 

// UDPSocket sock;
// string servAddress = "192.168.44.100"; // First arg: server address
// unsigned short servPort = Socket::resolveService("10001", "udp");

// int jpegqual =  70;

// int main(int argc, char *argv[])
// {
//     stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,1,"/dev/video0", vendor},
//                                     };

//     std::shared_ptr<nvCam> cameras[USED_CAMERA_NUM];
//     for(int i=0;i<USED_CAMERA_NUM;i++)
//         cameras[i].reset(new nvCam(camcfgs[i]));

//     std::vector<std::thread> threads;
//     for(int i=0;i<USED_CAMERA_NUM;i++)
//         threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
//     for(auto& th:threads)
//         th.detach();

//     vector<cv::Mat> imgs;
//     cv::Mat send;

//     vector < uchar > encoded;
//     cv::Mat img7, img8, ret;
//     while(1)
//     {
//         cameras[0]->getFrame(img7);

//         // cv::hconcat(vector<Mat>{img7, img8}, ret);
//         // cout << "7 s:" << img7.size()<<",8 s:"<<img8.size()<<".ret s:"<<ret.size()<<endl;
//         cv::imshow("1", img7);
//         // cv::waitKey(1);

//         //send to master
//         vector < int > compression_params;
//         compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
//         compression_params.push_back(jpegqual);

//         send = img7.clone();
//         // send = imread("final.png");

//         imencode(".jpg", send, encoded, compression_params);
//         // cv::imwrite("send.png", send);
//         int total_pack = 1 + (encoded.size() - 1) / SLAVE_PCIE_UDP_PACK_SIZE;
//         cout << "encodeed size:" << encoded.size() << "total_pack:" << total_pack << endl;

//         int ibuf[1];
//         ibuf[0] = total_pack;
//         sock.sendTo(ibuf, sizeof(int), servAddress, servPort);
//         cout << "before send data, send:" << total_pack << endl;
//         for (int i = 0; i < total_pack; i++)
//         {
//             sock.sendTo( & encoded[i * SLAVE_PCIE_UDP_PACK_SIZE], SLAVE_PCIE_UDP_PACK_SIZE, servAddress, servPort);
//             cout << "send  to :" << SLAVE_PCIE_UDP_PACK_SIZE << endl;
//         }

//         // waitKey(1);
        
//         if(argc > 1)
//         {
//             cv::imwrite("final.png", ret);
//             return 0;
//         }
//     }
//     return 0;
// }

#include <opencv2/opencv.hpp>
#include <thread>
#include <string>
// #include "ocvstitcher.hpp"
#include "PracticalSocket.h"
#include "nvcam.hpp"
#include "stitcherglobal.h"

#define USED_CAMERA_NUM 2
#define CAMERA_NUM 2

UDPSocket sock;
string servAddress = "192.168.44.100"; // First arg: server address
unsigned short servPort = Socket::resolveService("10001", "udp");

int jpegqual =  70;

int main(int argc, char *argv[])
{
    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,1,"/dev/video0", vendor},
                                    stCamCfg{camSrcWidth,camSrcHeight,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,2,"/dev/video1", vendor},
                                    };

    std::shared_ptr<nvCam> cameras[USED_CAMERA_NUM];
    for(int i=0;i<USED_CAMERA_NUM;i++)
        cameras[i].reset(new nvCam(camcfgs[i]));

    std::vector<std::thread> threads;
    for(int i=0;i<USED_CAMERA_NUM;i++)
        threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
    for(auto& th:threads)
        th.detach();

    vector<cv::Mat> imgs;
    cv::Mat send;

    vector < uchar > encoded;
    cv::Mat img7, img8, ret;
    while(1)
    {
        cameras[0]->getFrame(img7);
        cameras[1]->getFrame(img8);

        cv::hconcat(vector<cv::Mat>{img7, img8}, ret);
        // cout << "7 s:" << img7.size()<<",8 s:"<<img8.size()<<".ret s:"<<ret.size()<<endl;
        // cv::imshow("1", img7);
        // cv::waitKey(1);

        //send to master
        vector < int > compression_params;
        compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
        compression_params.push_back(jpegqual);

        send = ret.clone();
        // send = imread("final.png");

        imencode(".jpg", send, encoded, compression_params);
        // cv::imwrite("send.png", send);
        int total_pack = 1 + (encoded.size() - 1) / SLAVE_PCIE_UDP_PACK_SIZE;
        cout << "encodeed size:" << encoded.size() << "total_pack:" << total_pack << endl;

        int ibuf[1];
        ibuf[0] = total_pack;
        sock.sendTo(ibuf, sizeof(int), servAddress, servPort);
        cout << "before send data, send:" << total_pack << endl;
        for (int i = 0; i < total_pack; i++)
        {
            sock.sendTo( & encoded[i * SLAVE_PCIE_UDP_PACK_SIZE], SLAVE_PCIE_UDP_PACK_SIZE, servAddress, servPort);
            cout << "send  to :" << SLAVE_PCIE_UDP_PACK_SIZE << endl;
        }

        // waitKey(1);
        
        if(argc > 1)
        {
            cv::imwrite("final.png", ret);
            return 0;
        }
    }
    return 0;
}