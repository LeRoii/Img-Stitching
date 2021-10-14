#ifndef _IMAGE_PROCESS_H_
#define _IMAGE_PROCESS_H_

#include <fstream>
#include <opencv2/opencv.hpp>
#include <iostream>
#include "jetsonEncoder.h"
#include <vector>

#include <linux/can.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <utility>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <vector>
#include <sstream>
#include <netinet/in.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "yolo_v2_class.hpp"
#include "Yolo3Detection.h"

typedef struct
{
    bool use_dehaze; //电子去雾开关
    bool use_ssr; //图像增强开关
    bool bright_method; //亮度调节模式，1为手动，0为自动
    unsigned char bright;
    bool contrast_method; //对比度调节摸索，1为手动，0为自动
    unsigned char contrast; 
    bool use_flip;  //图像翻转开关
    bool use_detect; //图像十八开关
    bool use_cross; //电十字加载/消隐，1为加载，0为消隐
    bool video_save;    //视频存储开关
    bool self_check;    //自检开关
    bool open_window; //开窗局部放大开关
    bool turn_ctl;  //转台控制开关
    int turn_ctl_angle;
} canCmd;

class imageProcessor
{
    public:
    imageProcessor();
    cv::Mat Process(cv::Mat img);
    void publishImage(cv::Mat img);
    cv::Mat SSR(cv::Mat input);
    controlData getCtlCommand();

    private:
    cv::Mat channel_process(cv::Mat R);
    cv::Mat getROIimage(cv::Mat srcImg);
    cv::Mat ImageDetect(cv::Mat img, std::vector<int> &detret);
    void cut_img(cv::Mat src_img,std::vector<cv::Mat> &ceil_img);
    cv::Mat processImage(std::vector<cv::Mat> ceil_img);

};

#endif