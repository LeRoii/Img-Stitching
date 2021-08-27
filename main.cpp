// #include <opencv2/opencv.hpp>
#include "stitcher.hpp"
using namespace cv;
using namespace std;
int main()
{
    Mat imageRight, imageLeft, homo, ret;

    imageRight = imread("/space/code/Img-Stitching/imgs/11.png", 1);    //右图
    imageLeft  = imread("/space/code/Img-Stitching/imgs/22.png", 1);    //左图
    // imwrite("5.png", imageRight);
    // imwrite("6.png", imageLeft);
    // imageRight = imread("/space/code/Img-Stitching/imgs/01.png", 1);    //右图
    // imageLeft  = imread("/space/code/Img-Stitching/imgs/02.png", 1);    //左图

    // cv::resize(imageRight, imageRight, cv::Size(1920,1080));
    // cv::resize(imageLeft, imageLeft, cv::Size(1920,1080));

    // imwrite("33.png", imageRight);
    // imwrite("44.png", imageLeft);

    stitcherCfg cfg;
    cfg.imgHeight = 1944;
    cfg.imgWidth = 2592;
    cfg.imgHeight = 1080;
    cfg.imgWidth = 1920;

    stitcher imgst(cfg, 2);
    imgst.processnodet(imageRight, imageLeft, ret);

    auto startTime= std::chrono::steady_clock::now();
#if DEBUG
    int fps = 1;
#else
    int fps = 30;
#endif
    for(int i=0;i<fps;i++)
        imgst.processnodet(imageRight, imageLeft, ret);

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    cout<<"1 frame SpendTime = "<<  duration.count()/fps <<"ms"<<endl;

    return 0;

}