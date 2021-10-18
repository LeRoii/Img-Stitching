#include <chrono>
#include <opencv2/opencv.hpp>
#include "ocvstitcher.hpp"

// #include "imageProcess.h"

// void Stringsplit(string str, const char split, vector<string>& res)
// {
//     istringstream iss(str);	// 输入流
//     string token;			// 接收缓冲区
//     while (getline(iss, token, split))	// 以split为分隔符
//     {
//         res.push_back(token);
//     }
// }

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

    // imageProcessor nvProcessor;
    // Mat img = imread("1.jpg");
    // resize(img,img,Size(480,270));
    // std::vector<int> detret;
    // Mat yoloRet = nvProcessor.ImageDetect(img, detret);
    // imwrite("1-ret.png",yoloRet);

    // Mat mt = (cv::Mat_<double>(3,3) << 853.417882746302, 0, 483.001902270090,
    //                     0, 959.666714085956, 280.450178308760,
    //                     0, 0, 1);

    // MatConstIterator_<double> it1 = mt.begin<double>(), it1_end = mt.end<double>();
    // for( ; it1 != it1_end; ++it1)
    //     LOGLN("iterate:::" << *(it1));

    // ocvStitcher ostitcherUp(960/2, 540/2);
    // vector<Mat> upImgs(4);

    // upImgs.clear();
    // Mat img = imread("/home/nvidia/ssd/code/0929IS/2222/1.png");
    // // resize(img, img, Size(960/2, 540/2));
    // upImgs.push_back(img);
    // img = imread("/home/nvidia/ssd/code/0929IS/2222/2.png");
    // // resize(img, img, Size(960/2, 540/2));
    // upImgs.push_back(img);
    // img = imread("/home/nvidia/ssd/code/0929IS/2222/3.png");
    // // resize(img, img, Size(960/2, 540/2));
    // upImgs.push_back(img);
    // img = imread("/home/nvidia/ssd/code/0929IS/2222/4.png");
    // // resize(img, img, Size(960/2, 540/2));
    // upImgs.push_back(img);

    // ostitcherUp.init(upImgs);



    // std::ifstream fin("camerapara.txt", std::ios::in);
    // Mat cameraK = Mat(Size(3,3), CV_64FC1);
    // vector<Mat> cameraR;
    // for(int i=0;i<4;i++)
    //     cameraR.push_back(Mat(Size(3,3), CV_32FC1));

    // vector<string> res;
    // if(fin.is_open())
    // {
    //     cout<<"in if"<<endl;
    //     std::string str;
    //     fin >> str;
    //     cout<<str<<endl;
    //     Stringsplit(str, ',', res);
    //     for(int i=0;i<3;i++)
    //     {
    //         for(int j=0;j<3;j++)
    //         {
    //             cameraK.at<double>(i,j) = stod(res[i*3+j]);
    //         }
    //     }
    //     cout<<cameraK<<endl;

    //     for(int i=0;i<4;i++)
    //     {
    //         fin >> str;
    //         cout<<str<<endl;
    //         res.clear();
    //         Stringsplit(str, ',', res);
    //         for(int mi=0;mi<3;mi++)
    //         {
    //             for(int mj=0;mj<3;mj++)
    //             {
    //                 cameraR[i].at<float>(mi,mj) = stod(res[mi*3+mj]);
    //             }
    //         }

    //         cout<<cameraR[i]<<endl;
    //     }
    // }

    // std::chrono::system_clock::time_point now = chrono::system_clock::now();//当前时间time_point格式
    // std::time_t oldTime = time(nullptr);//c函数获取当前时间
    // cout << "oldTime = " << oldTime << endl;
    // chrono::system_clock::time_point timePoint = chrono::system_clock::now();//stl库获取当前时间
    // std::time_t newTime = chrono::system_clock::to_time_t(timePoint);//转换为旧式接口，单位:秒
    // cout<<"newTime = " << newTime <<endl;// oldTime == timeT


    // std::time_t nowTime = chrono::system_clock::to_time_t(now);//转换为 std::time_t 格式 
    // std::put_time(std::localtime(&nowTime), "%Y-%m-%d %X"); // 2019-06-18 14:25:56
    
    // // !std::localtime非线程安全，使用localtime_r函数代替
    // struct tm cutTm = {0};
    // std::put_time(localtime_r(&nowTime, &cutTm), "%Y-%m-%d %X");// 2019-06-18 14:25:56

    // std::chrono::system_clock::time_point now = chrono::system_clock::now();
    // time_t tt = chrono::system_clock::to_time_t(now);
    // cout << "string format: " << ctime(&tt) << endl;

    std::time_t tt = chrono::system_clock::to_time_t (chrono::system_clock::now());
    struct std::tm * ptm = std::localtime(&tt);
    std::cout << "Now (local time): " << std::put_time(ptm,"%F-%H-%M-%S") << '\n';
           
    return 0;
}