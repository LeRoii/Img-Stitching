/********* test detector *********/
// #include "imageProcess.h"
// // #include <opencv2/opencv.hpp>
// #include <iostream>
// int main()
// {
//     std::string net = "/home/nvidia/ssd/model/yolo4_berkeley_fp16.rt";
//     imageProcessor nvProcessor(net);
//     // cv::Mat img = cv::imread("/home/nvidia/ssd/data/1.jpg");
//     cv::Mat img = cv::imread("/home/nvidia/ssd/data/7.png");
//     if (img.empty()) //check whether the image is loaded or not
//     {
//         std::cout << "Error : Image cannot be loaded..!!" << std::endl;
//         //system("pause"); //wait for a key press
//         return -1;
//     }
//     // cv::imshow("1", img);
//     // cv::waitKey(0);
//     // cv::imwrite("3.jpg", img);
//     // cv::Mat croped = img(cv::Rect(640, 300, 640, 480));
//     std::vector<int> detret;
//     int cnt = 0;
//     cv::Mat ret = nvProcessor.ImageDetect(img, detret);
//     // cnt++;
//     cv::imshow("1", ret);
//     cv::waitKey(1);

//     cv::imwrite("det.png", ret);
//     // cv::waitKey(0);

//     return 0;

// }


/********* test binding mac address *********/
/*
#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/utsname.h>
int main()
{
   struct utsname buf1;
   errno =0;
   if(uname(&buf1)!=0)
   {
      perror("uname doesn't return 0, so there is an error");
      exit(EXIT_FAILURE);
   }
   printf("System Name = %s\n", buf1.sysname);
   printf("Node Name = %s\n", buf1.nodename);
   printf("Version = %s\n", buf1.version);
   printf("Release = %s\n", buf1.release);
   printf("Machine = %s\n", buf1.machine);
}
*/

// #include <string>
// #include <string.h>
// #include <net/if.h>
// #include <sys/ioctl.h>

// using namespace std;

// int _System(const char * cmd, char *pRetMsg, int msg_len)
// {
// 	FILE * fp;
// 	char * p = NULL;
// 	int res = -1;
// 	if (cmd == NULL || pRetMsg == NULL || msg_len < 0)
// 	{
// 		printf("Param Error!\n");
// 		return -1;
// 	}
// 	if ((fp = popen(cmd, "r") ) == NULL)
// 	{
// 		printf("Popen Error!\n");
// 		return -2;
// 	}
// 	else
// 	{
// 		memset(pRetMsg, 0, msg_len);
// 		//get lastest result
// 		while(fgets(pRetMsg, msg_len, fp) != NULL)
// 		{
// 			printf("Msg:%s",pRetMsg); //print all info
// 		}
 
// 		if ( (res = pclose(fp)) == -1)
// 		{
// 			printf("close popenerror!\n");
// 			return -3;
// 		}
// 		pRetMsg[strlen(pRetMsg)-1] = '\0';
// 		return 0;
// 	}
// }

// void get_mac(char * mac_a)
// {
//     int                 sockfd;
//     struct ifreq        ifr;

//     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sockfd == -1) {
//         perror("socket error");
//         exit(1);
//     }
//     strncpy(ifr.ifr_name, "eth1", IFNAMSIZ);      //Interface name

//     if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == 0) {  //SIOCGIFHWADDR 获取hardware address
//         memcpy(mac_a, ifr.ifr_hwaddr.sa_data, 6);
//     }
// }

// int main()
// {
//     // string str1 = "cat /sys/class/net/eth0/address";
//     // const char *command1 = str1.c_str();     //c_str() converts the string into a C-Style string
//     // system(command1);
//     char buf[100];
//     int msg_len = 100;
//     // _System("cat /sys/class/net/eth0/address", buf, msg_len);
//     char * this_mac = new char[6];
//     get_mac(this_mac);
//     printf("mac: %02x:%02x:%02x:%02x:%02x:%02x\n", this_mac[0]&0xff, this_mac[1]&0xff, this_mac[2]&0xff, this_mac[3]&0xff, this_mac[4]&0xff, this_mac[5]&0xff);

// }

/********* test yaml *********/
// #include "yaml-cpp/yaml.h"

// int main()
// {
//     YAML::Node config = YAML::LoadFile("yamlpath");
//     return 0;
// }

/********* test nvidia timer *********/
// #include <unistd.h> //sleep usleep
// #include <stdio.h>
// #include "helper_timer.h"

// int main()
// {
//     StopWatchInterface *timer = NULL;
//     sdkCreateTimer(&timer);
//     sdkResetTimer(&timer);
//     sdkStartTimer(&timer);

//     for(int i=0;i<50;i++)
//     {
//         sdkResetTimer(&timer);
//         printf("loop:%d\n",i);
//         usleep(1000*300);

//         // sdkStopTimer(&timer);
//         // printf("sdkGetAverageTimerValue:%f ms\n", sdkGetAverageTimerValue(&timer));
//         printf("sdkGetTimerValue:%f ms\n", sdkGetTimerValue(&timer));

//         usleep(1000*500);

//         // sdkStopTimer(&timer);
//         // printf("sdkGetAverageTimerValue:%f ms\n", sdkGetAverageTimerValue(&timer));
//         printf("sdkGetTimerValue:%f ms\n", sdkGetTimerValue(&timer));
//     }

//     return 0;
// }

/********* test opencv video capture *********/
// #include <opencv2/opencv.hpp>
// #include <iostream>
// #include <opencv2/core/cuda.hpp>
// #include <opencv2/cudaimgproc.hpp> 
// using namespace std;
// using namespace cv;
// int main(int argc, char **argv)
// {
//     cv::cuda::GpuMat a;
// 	VideoCapture cap;
// 	cap.open(0);
//     cap.set(CV_CAP_PROP_MODE,CV_CAP_MODE_YUYV);
// 	while (1)
// 	{
// 		Mat frame;//定义一个变量把视频源一帧一帧显示
// 		cap >> frame;
// 		if (frame.empty())
// 		{
// 			cout << "Finish" << endl;
// 			break;
// 		}
// 		imshow("Input video", frame);
// 		waitKey(30);
// 	}
// 	cap.release();
// 	return 0;
 
// }

/********* test nvcam *********/
#include <thread>
#include <memory>
#include <opencv2/core/utility.hpp>
#include "yaml-cpp/yaml.h"
#include "nvcam.hpp"
#include "PracticalSocket.h"
// #include "ocvstitcher.hpp"
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
    spdlog::set_level(spdlog::level::trace);
    

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

    // cameras[idx-1]->start_capture();

    while(1)
    {
        cameras[idx-1]->read_frame();
        // cameras[idx-1]->ctx.renderer->render(cameras[idx-1]->retNvbuf[cameras[idx-1]->distoredszIdx].dmabuff_fd);
        // imshow("a", cameras[idx-1]->m_ret);
        // waitKey(1);
    }
    return 0;
}

/********* test yuv 2 rgb *********/
// #include <opencv2/opencv.hpp>
// #include <fcntl.h>
// #include <stdio.h>
// #include <unistd.h>

// int main()
// {
//    // int img = open("./camera.YUYV", O_RDONLY);
//    //  if (-1 == img)
//    //      printf("Failed to open file for rendering");
//    //  int bufsize = 1920*1080*2;
//    //  unsigned char *buf = (unsigned char*)malloc(bufsize);
//    //  int cnt = read(img, buf, bufsize);
//    //  printf("read %d bytes\n", cnt);
//    //  cv::Mat mt(1080,1920,CV_8UC2);

//    //  memcpy(mt.data, buf, 1920 * 1080 * 2 * sizeof(unsigned char));

//    //  cv::Mat rgbi;
//    //  cv::cvtColor(mt, rgbi, cv::COLOR_YUV2BGR_YUYV);
//    //  cv::imshow("11", rgbi);
//    //  cv::waitKey(0);

//    // cv::Mat i420;
//    // cv::cvtColor(rgbi, i420, cv::COLOR_RGB2YUV_I420);
//    // int ii = open("i420", O_CREAT | O_WRONLY | O_APPEND | O_TRUNC,
//    //          S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
//    // if(-1 == write(ii, i420.data, 1920*1080*3/2))
//    // {
//    //    printf("aaaaaa\n");
//    // }

//    // close(ii);

//    int img = open("i420", O_RDONLY);
//     if (-1 == img)
//         printf("Failed to open file for rendering");
//     int bufsize = 1920*1080*3/2;
//     unsigned char *buf = (unsigned char*)malloc(bufsize);
//     int cnt = read(img, buf, bufsize);
//     printf("read %d bytes\n", cnt);
//     cv::Mat mt(1080*3/2,1920,CV_8UC1);

//     memcpy(mt.data, buf, 1920 * 1080 * 3/2 * sizeof(unsigned char));

//     cv::Mat rgbi;
//     cv::cvtColor(mt, rgbi, cv::COLOR_YUV2BGR_I420);
//     cv::imshow("11", rgbi);
//     cv::waitKey(0);




//     return 0;

// }