#ifndef _IMAGE_PROCESS_H_
#define _IMAGE_PROCESS_H_

#include <fstream>
#include <opencv2/opencv.hpp>
#include <iostream>
#include "jetsonEncoder.h"

#include "yolo_v2_class.hpp"
#include "Yolo3Detection.h"



class imageProcessor
{
    public:
    imageProcessor();
    ~imageProcessor();
    cv::Mat Process(cv::Mat img);
    cv::Mat ImageDetect(cv::Mat img);

    private:
    void publishImage(cv::Mat &img);
    cv::Mat getROIimage(cv::Mat srcImg);
    
    void cut_img(cv::Mat &src_img,std::vector<cv::Mat> &ceil_img);
    tk::dnn::Yolo3Detection detNN;
    jetsonEncoder nvEncoder;
};

#endif