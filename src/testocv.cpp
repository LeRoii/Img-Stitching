#include <opencv2/opencv.hpp>
#include "ocvstitcher.hpp"

#include "imageProcess.h"

int main()
{
    // ocvStitcher ostitcher(960/2, 540/2);
    // vector<Mat> imgs;

    // // imgs.push_back(imread("../tmp/1-dist.png"));
    // // imgs.push_back(imread("../tmp/2-dist.png"));
    // // imgs.push_back(imread("../tmp/3-dist.png"));
    // // imgs.push_back(imread("../tmp/4-dist.png"));

    // imgs.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/1.png"));
    // imgs.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/2.png"));
    // imgs.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/3.png"));
    // imgs.push_back(imread("/home/nvidia/ssd/code/0929IS/2222/4.png"));

    // for(int i=0;i<1;i++)
    //     ostitcher.init(imgs);

    // Mat ret;
    // ostitcher.process(imgs, ret);

    // for(int i=0;i<4;i++)
    // {
    //     resize(imgs[i], imgs[i], Size(), 0.5, 0.5, INTER_LINEAR_EXACT);
    // }

    // imwrite("1.png", imgs[0]);
    // imwrite("2.png", imgs[1]);
    // imwrite("3.png", imgs[2]);
    // imwrite("4.png", imgs[3]);

    // Mat img = imread("/home/nvidia/ssd/code/0929IS/build/1-blenderMask.png");
    // img(Rect(img.size().width*0.7,0,img.size().width*0.3,img.size().height)).setTo(255);
    // imwrite("1blendm.png", img);

    imageProcessor nvProcessor;
    Mat img = imread("1.jpg");
    resize(img,img,Size(480,270));
    std::vector<int> detret;
    Mat yoloRet = nvProcessor.ImageDetect(img, detret);
    imwrite("1-ret.png",yoloRet);

    return 0;
}