#ifndef _IMAGE_PROCESS_H_
#define _IMAGE_PROCESS_H_

#include <fstream>
#include <opencv2/opencv.hpp>
#include <iostream>
#include "jetsonEncoder.h"

#include "yolo_v2_class.hpp"
#include "Yolo3Detection.h"



class imagePorcessor
{
    public:
    imagePorcessor();
    ~imagePorcessor();
    cv::Mat Process(cv::Mat img);

    private:
    void publishImage(cv::Mat &img);
    cv::Mat getROIimage(cv::Mat srcImg);
    cv::Mat ImageDetect(cv::Mat img);
    void cut_img(cv::Mat src_img,std::vector<cv::Mat> &ceil_img);

};

#endif