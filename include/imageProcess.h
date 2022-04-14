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
// #include "cansender.hpp"
#include "yolo_v2_class.hpp"
#include "Yolo3Detection.h"

#include "spdlog/spdlog.h"

struct canCmd
{
    bool use_dehaze; //电子去雾开关
    bool use_ssr; //图像增强开关
    bool bright_method; //亮度调节模式，1为手动，0为自动
    unsigned char bright;
    bool contrast_method; //对比度调节摸索，1为手动，0为自动
    unsigned char contrast; 
    bool use_flip;  //图像翻转开关
    bool use_detect; //图像识别开关
    bool use_cross; //电十字加载/消隐，1为加载，0为消隐
    bool video_save;    //视频存储开关
    bool self_check;    //自检开关
    bool open_window; //开窗局部放大开关
    bool turn_ctl;  //转台控制开关
    int turn_ctl_angle;
};

struct stObject
{
    int cls;
    float x,y,w,h;
    float conf;
};

class imageProcessor
{
    public:
    imageProcessor(std::string net, std::string canname = "can0", int batchsize = 1);
    cv::Mat Process(cv::Mat &img);
    cv::Mat ProcessOnce(cv::Mat &img);
    void publishImage(cv::Mat img);     //图像h264编码、UDP发送和视频流存储
    cv::Mat SSR(cv::Mat input);     //图像增强
    controlData getCtlCommand();    //UDP通讯获得上位机控制命令
    cv::Mat ImageDetect(cv::Mat &img, std::vector<int> &detret);     //目标检测
    void ImageDetect(std::vector<cv::Mat> &imgs, std::vector<std::vector<int>> &detret);

    private:
    cv::Mat channel_process(cv::Mat R);     //对图像单通道进行增强
    cv::Mat getROIimage(cv::Mat srcImg);    //不改变原图宽高比的情况下，将图像填充成方形
    
    void cut_img(cv::Mat &src_img, std::vector<cv::Mat> &ceil_img);  //将拼接好的图像裁成2*1图像块并存储到vector中
    cv::Mat processImage(std::vector<cv::Mat> &ceil_img);    //调用图像裁剪、检测和检测结果拼接、UDP发送目标信息、CAN发送目标信息
    tk::dnn::Yolo3Detection detNN;
    jetsonEncoder nvEncoder;
    // cansender *pCanSender;
    int n_batch;
};

#endif