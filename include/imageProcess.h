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

#include "yolo_v2_class.hpp"
#include "Yolo3Detection.h"



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