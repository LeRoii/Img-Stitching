#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <iostream>
#include <chrono> 

#include "stitcher.hpp"
using namespace cv;
using namespace std;

void imgStitch(const cv::Mat &leftimg, const cv::Mat &rightimg, cv::Mat &ret);

int main(int argc, char *argv[])
{
    Mat imageRight = imread("./3-undist.png", 1);    //右图
    Mat imageLeft = imread("./2-undist.png", 1);    //左图

    // cv::resize(imageRight, imageRight, cv::Size(1920,1080));
    // cv::resize(imageLeft, imageLeft, cv::Size(1920,1080));
    // cv::resize(imageRight, imageRight, cv::Size(960,540));
    // cv::resize(imageLeft, imageLeft, cv::Size(960,540));

    // cv::imwrite("11.png", imageRight);
    // cv::imwrite("22.png", imageLeft);

    // return 0;

    auto startTime = std::chrono::steady_clock::now();

    //灰度图转换  
    Mat imageRightGray, imageLeftGray;
    cvtColor(imageRight, imageRightGray, CV_RGB2GRAY);
    cvtColor(imageLeft, imageLeftGray, CV_RGB2GRAY);

    cv::Mat mask = cv::Mat::zeros(cv::Size(960, 540), CV_8UC1);
    mask(cv::Rect(0, 0, 480, 540)).setTo(255);
    cv::Mat maskR = mask.clone();
    mask.setTo(0);
    mask(cv::Rect(960 - 480, 0, 480, 540)).setTo(255).clone();
    cv::Mat maskL = mask.clone();

    //提取特征点    
    int minHessian = 400;
    cv::Ptr<cv::xfeatures2d::SURF> Detector = cv::xfeatures2d::SURF::create(minHessian);
    vector<KeyPoint> keyPointRight, keyPointLeft;
    // Detector->detect(imageRightGray, keyPointRight);
    // Detector->detect(imageLeftGray, keyPointLeft);

    //特征点描述，为下边的特征点匹配做准备    
    Mat imageDescRight, imageDescLeft;
    // Detector->compute(imageRightGray, keyPointRight, imageDescRight);
    // Detector->compute(imageLeftGray, keyPointLeft, imageDescLeft);

    Detector->detectAndCompute(imageRightGray, maskR, keyPointRight, imageDescRight);
    Detector->detectAndCompute(imageLeftGray, maskL, keyPointLeft, imageDescLeft);

    cout<<"right key pt size:"<<keyPointRight.size()<<endl;
    cout<<"left key pt size:"<<keyPointLeft.size()<<endl;

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    cout<<"detectAndCompute SpendTime = "<<  duration.count() <<"ms"<<endl;

    cv::Ptr<cv::BFMatcher> matcher = cv::BFMatcher::create(NORM_L2);
    // cv::Ptr<cv::FlannBasedMatcher> matcher = cv::FlannBasedMatcher::create();
    vector<vector<DMatch> > knnMatchePoints;
    // vector<DMatch> matchePoints;
    vector<DMatch> GoodMatchePoints;

    //useless for bfmatcher
    // vector<Mat> train_desc(1, imageDescRight);
    // matcher->add(train_desc);
    // matcher->train();
    // matcher->knnMatch(imageDescLeft, matchePoints, 2);

    // matcher->match(imageDescLeft, imageDescRight, GoodMatchePoints);
    matcher->knnMatch(imageDescLeft, imageDescRight, knnMatchePoints, 2);
    // cout << "total match points: " << matchePoints.size() << endl;
    cout << "total knn match points: " << knnMatchePoints.size() << endl;

    // // Lowe's algorithm,获取优秀匹配点
    for (int i = 0; i < knnMatchePoints.size(); i++)
    {
        if (knnMatchePoints[i][0].distance < 0.5 * knnMatchePoints[i][1].distance)
        {
            GoodMatchePoints.push_back(knnMatchePoints[i][0]);
        }
    }

    cout << "knn GoodMatchePoints: " << GoodMatchePoints.size() << endl;

    //for hamming dist, useless for sift and surf
    // double min_dist = 1000, max_dist = 0;
    // // 找出所有匹配之间的最大值和最小值
    // for (int i = 0; i < matchePoints.size(); i++)
    // {
    //     double dist = matchePoints[i].distance;
    //     if (dist < min_dist) min_dist = dist;
    //     if (dist > max_dist) max_dist = dist;
    // }

    // cout << "max_dist:" << max_dist << ", min dist:" << min_dist << endl;
    // // 当描述子之间的匹配不大于2倍的最小距离时，即认为该匹配是一个错误的匹配。
    // // 但有时描述子之间的最小距离非常小，可以设置一个经验值作为下限
    // for (int i = 0; i < matchePoints.size(); i++)
    // {
    //     if (matchePoints[i].distance <= 2 * min_dist)
    //         GoodMatchePoints.push_back(matchePoints[i]);
    // }

    cout << "GoodMatchePoints: " << GoodMatchePoints.size() << endl;

    vector<Point2f> imagePointsRight, imagePointsLeft;

    for (int i = 0; i<GoodMatchePoints.size(); i++)
    {
        imagePointsLeft.push_back(keyPointLeft[GoodMatchePoints[i].queryIdx].pt);
        imagePointsRight.push_back(keyPointRight[GoodMatchePoints[i].trainIdx].pt);
    }

    endTime = std::chrono::steady_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    cout<<"match SpendTime = "<<  duration.count() <<"ms"<<endl;

    //获取图像1到图像2的投影映射矩阵 尺寸为3*3  
    Mat homo = findHomography(imagePointsRight, imagePointsLeft, CV_RANSAC);
    ////也可以使用getPerspectiveTransform方法获得透视变换矩阵，不过要求只能有4个点，效果稍差  
    //Mat   homo=getPerspectiveTransform(imagePointsRight,imagePointsLeft);  
    cout << "homo：\n" << homo << endl << endl; //输出映射矩阵   

    ofstream fout("Homography.txt", std::ofstream::out);
    fout << homo;
    fout.close();

   //计算配准图的四个顶点坐标
    CalcCorners(homo, imageRight);
    cout << "left_top:" << corners.left_top << endl;
    cout << "left_bottom:" << corners.left_bottom << endl;
    cout << "right_top:" << corners.right_top << endl;
    cout << "right_bottom:" << corners.right_bottom << endl;

    //图像配准  
    Mat imageTransformRight, imageTransformLeft;
    // warpPerspective(imageRight, imageTransformRight, homo, Size(MAX(corners.right_top.x, corners.right_bottom.x), imageLeft.rows));
    warpPerspective(imageRight, imageTransformRight, homo, Size(imageLeft.cols*2, imageLeft.rows));
    //warpPerspective(imageRight, imageTransformLeft, adjustMat*homo, Size(imageLeft.cols*1.3, imageLeft.rows*1.8));
    
    //创建拼接后的图,需提前计算图的大小
    int dst_width = imageTransformRight.cols;  //取最右点的长度为拼接图的长度
    int dst_height = imageLeft.rows;

    Mat dst(dst_height, dst_width, CV_8UC3);
    dst.setTo(0);

    imageTransformRight.copyTo(dst(Rect(0, 0, imageTransformRight.cols, imageTransformRight.rows)));
    imageLeft.copyTo(dst(Rect(0, 0, imageLeft.cols, imageLeft.rows)));

    imwrite("b_dst.jpg", dst);
    OptimizeSeam(imageLeft, imageTransformRight, dst);
    imwrite("dst.jpg", dst);

#if 1
    endTime = std::chrono::steady_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    cout<<"SpendTime = "<<  duration.count() <<"ms"<<endl;

    Mat first_match;
    drawMatches(imageLeft, keyPointLeft, imageRight, keyPointRight, GoodMatchePoints, first_match);
    imwrite("first_match.png ", first_match);
    imwrite("trans1.jpg", imageTransformRight);
    imwrite("dst.jpg", dst);
#endif

    return 0;
}