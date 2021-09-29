#include <opencv2/opencv.hpp>
#include "ocvstitcher.hpp"

int main()
{
    ocvStitcher ostitcher(960/2, 540/2);
    vector<Mat> imgs;

    // imgs.push_back(imread("../tmp/1-dist.png"));
    // imgs.push_back(imread("../tmp/2-dist.png"));
    // imgs.push_back(imread("../tmp/3-dist.png"));
    // imgs.push_back(imread("../tmp/4-dist.png"));

    imgs.push_back(imread("../tmp/1.png"));
    imgs.push_back(imread("../tmp/2.png"));
    imgs.push_back(imread("../tmp/3.png"));
    imgs.push_back(imread("../tmp/4.png"));

    ostitcher.init(imgs);

    Mat ret;
    ostitcher.process(imgs, ret);

    // for(int i=0;i<4;i++)
    // {
    //     resize(imgs[i], imgs[i], Size(), 0.5, 0.5, INTER_LINEAR_EXACT);
    // }

    // imwrite("1.png", imgs[0]);
    // imwrite("2.png", imgs[1]);
    // imwrite("3.png", imgs[2]);
    // imwrite("4.png", imgs[3]);

    return 0;
}