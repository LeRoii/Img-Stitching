#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <iostream>
#include <chrono> 

#include "stitcher.hpp"
using namespace cv;
using namespace std;

void imgStitch(const cv::Mat &leftimg, const cv::Mat &rightimg, cv::Mat &ret);

int main1(int argc, char *argv[])
{
    Mat imageRight = imread("/space/workspace/stitching/01.png", 1);    //右图
    Mat imageLeft = imread("/space/workspace/stitching/02.png", 1);    //左图

    // cv::resize(imageRight, imageRight, cv::Size(960/2,540/2));
    // cv::resize(imageLeft, imageLeft, cv::Size(960/2,540/2));

    // cv::imwrite("11.png", imageRight);
    // cv::imwrite("22.png", imageLeft);

    // return 0;

    auto startTime = std::chrono::steady_clock::now();

    //灰度图转换  
    Mat imageRightGray, imageLeftGray;
    cvtColor(imageRight, imageRightGray, CV_RGB2GRAY);
    cvtColor(imageLeft, imageLeftGray, CV_RGB2GRAY);

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

    Detector->detectAndCompute(imageRightGray, cv::Mat(), keyPointRight, imageDescRight);
    Detector->detectAndCompute(imageLeftGray, cv::Mat(), keyPointLeft, imageDescLeft);

    cout<<"right key pt size:"<<keyPointRight.size()<<endl;
    cout<<"left key pt size:"<<keyPointLeft.size()<<endl;

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    cout<<"detectAndCompute SpendTime = "<<  duration.count() <<"ms"<<endl;

    FlannBasedMatcher matcher;
    vector<vector<DMatch> > matchePoints;
    vector<DMatch> GoodMatchePoints;

    vector<Mat> train_desc(1, imageDescRight);
    matcher.add(train_desc);
    matcher.train();

    matcher.knnMatch(imageDescLeft, matchePoints, 2);
    cout << "total match points: " << matchePoints.size() << endl;

    // Lowe's algorithm,获取优秀匹配点
    for (int i = 0; i < matchePoints.size(); i++)
    {
        if (matchePoints[i][0].distance < 0.4 * matchePoints[i][1].distance)
        {
            GoodMatchePoints.push_back(matchePoints[i][0]);
        }
    }

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

   //计算配准图的四个顶点坐标
    CalcCorners(homo, imageRight);
    cout << "left_top:" << corners.left_top << endl;
    cout << "left_bottom:" << corners.left_bottom << endl;
    cout << "right_top:" << corners.right_top << endl;
    cout << "right_bottom:" << corners.right_bottom << endl;

    //图像配准  
    Mat imageTransformRight, imageTransformLeft;
    warpPerspective(imageRight, imageTransformRight, homo, Size(MAX(corners.right_top.x, corners.right_bottom.x), imageLeft.rows));
    //warpPerspective(imageRight, imageTransformLeft, adjustMat*homo, Size(imageLeft.cols*1.3, imageLeft.rows*1.8));
    
    //创建拼接后的图,需提前计算图的大小
    int dst_width = imageTransformRight.cols;  //取最右点的长度为拼接图的长度
    int dst_height = imageLeft.rows;

    Mat dst(dst_height, dst_width, CV_8UC3);
    dst.setTo(0);

    imageTransformRight.copyTo(dst(Rect(0, 0, imageTransformRight.cols, imageTransformRight.rows)));
    imageLeft.copyTo(dst(Rect(0, 0, imageLeft.cols, imageLeft.rows)));

    // imwrite("b_dst.jpg", dst);
    OptimizeSeam(imageLeft, imageTransformRight, dst);
    imwrite("dst.jpg", dst);

#ifdef DEBUG
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

Mat getHomo(Mat& imageRight, Mat& imageLeft){
    //--------------------------------运用ORB算法和GPU(时间257ms)--------------------------
    
    cuda::GpuMat imageRight_Gpu, imageLeft_Gpu;
    cuda::GpuMat imageRightGray_Gpu, imageLeftGray_Gpu;
    cuda::GpuMat keyPtsRight_Gpu, keyPtsLeft_Gpu;
    cuda::GpuMat imageDescRight_Gpu, imageDescLeft_Gpu;
    vector<KeyPoint> keyPointRight, keyPointLeft;

    imageRight_Gpu.upload(imageRight);
    imageLeft_Gpu.upload(imageLeft);

	cuda::cvtColor(imageRight_Gpu, imageRightGray_Gpu, CV_RGB2GRAY);
	cuda::cvtColor(imageLeft_Gpu, imageLeftGray_Gpu, CV_RGB2GRAY);

    Ptr<cuda::ORB> orbDetector = cuda::ORB::create(10000);

    // orbDetector->detectAndCompute(imageRightGray_Gpu, cuda::GpuMat(), keyPointRight, imageDescRight_Gpu);
    // orbDetector->detectAndCompute(imageLeftGray_Gpu, cuda::GpuMat(), keyPointLeft, imageDescLeft_Gpu);

    orbDetector->detectAndComputeAsync(imageRightGray_Gpu, cuda::GpuMat(), keyPtsRight_Gpu, imageDescRight_Gpu);
    orbDetector->convert(keyPtsRight_Gpu, keyPointRight);
    
    orbDetector->detectAndComputeAsync(imageLeftGray_Gpu, cuda::GpuMat(), keyPtsLeft_Gpu, imageDescLeft_Gpu);
    orbDetector->convert(keyPtsLeft_Gpu, keyPointLeft);

    
    // auto endTime = std::chrono::steady_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    // cout<<"detectAndCompute SpendTime = "<<  duration.count() <<"ms"<<endl;
    
    //--------------------------------------------------
    
    Mat imageDescLeft(imageDescLeft_Gpu);
    Mat imageDescRight(imageDescRight_Gpu);
    vector<DMatch> GoodMatchePoints;
    vector<vector<DMatch> > matchePoints;
    
    cv::Ptr<cv::cuda::DescriptorMatcher> matcher = cv::cuda::DescriptorMatcher::createBFMatcher(cv::NORM_HAMMING);
    matcher->knnMatch(imageDescLeft_Gpu, imageDescRight_Gpu, matchePoints, 2);
    // matcher->knnMatchAsync(imageDescLeft_Gpu, imageDescRight_Gpu, )

    cout << "total match points: " << matchePoints.size() << endl;
    // Lowe's algorithm,获取优秀匹配点
    
    for (int i = 0; i < matchePoints.size(); i++)
    {
        if (matchePoints[i][0].distance < 0.6 * matchePoints[i][1].distance)
        {   
            GoodMatchePoints.push_back(matchePoints[i][0]);
        }
    }

    cout << "GoodMatchePoints: " << GoodMatchePoints.size() << endl;

    vector<Point2f> imagePointsRight, imagePointsLeft;

    for (int i = 0; i<GoodMatchePoints.size(); i++)
    {
        imagePointsLeft.push_back(keyPointLeft[GoodMatchePoints[i].queryIdx].pt);
        imagePointsRight.push_back(keyPointRight[GoodMatchePoints[i].trainIdx].pt);
    }

    Mat homo = findHomography(imagePointsRight, imagePointsLeft, CV_RANSAC);

    return homo;
}

void stitch(Mat& imageRight, Mat& imageLeft, Mat& ret)
{
    Mat homo = getHomo(imageRight, imageLeft);

    CalcCorners(homo, imageRight);

    //图像配准  
    Mat imageTransformRight;
    warpPerspective(imageRight, imageTransformRight, homo, Size(MAX(corners.right_top.x, corners.right_bottom.x), imageLeft.rows));
    // imwrite("../test/no_cut/trans1.jpg", imageTransformRight);

    //创建拼接后的图,需提前计算图的大小
    int dst_width = imageTransformRight.cols;  //取最右点的长度为拼接图的长度
    int dst_height = imageLeft.rows;

    Mat dst(dst_height, dst_width, CV_8UC3);

    dst.setTo(0);

    imageTransformRight.copyTo(dst(Rect(0, 0, imageTransformRight.cols, imageTransformRight.rows)));
    imageLeft.copyTo(dst(Rect(0, 0, imageLeft.cols, imageLeft.rows)));

    // imwrite("../test/no_cut/b_dst.jpg", dst);

    OptimizeSeam(imageLeft, imageTransformRight, dst);
}

int main()
{
    Mat imageRight, imageLeft, homo, ret;

    imageRight = imread("/space/workspace/stitching/01.png", 1);    //右图
    imageLeft  = imread("/space/workspace/stitching/02.png", 1);    //左图

    cv::resize(imageRight, imageRight, cv::Size(960,540));
    cv::resize(imageLeft, imageLeft, cv::Size(960,540));

    stitcherCfg cfg;
    cfg.imgHeight = 1080;
    cfg.imgWidth = 1920;

    stitcher imgst(cfg);
    imgst.process(imageRight, imageLeft, ret);

    auto startTime= std::chrono::steady_clock::now();

    
    for(int i=0;i<3;i++)
        imgst.process(imageRight, imageLeft, ret);

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    cout<<"1 frame SpendTime = "<<  duration.count()/30 <<"ms"<<endl;

    return 0;

}

int main2()
{
    Mat imageRight, imageLeft, homo, ret;

    imageRight = imread("/space/workspace/stitching/3.png", 1);    //右图
    imageLeft  = imread("/space/workspace/stitching/4.png", 1);    //左图

    cv::cuda::SURF_CUDA surf(400);
    cv::cuda::GpuMat imageRightGpu, imageLeftGpu;
    cv::cuda::GpuMat imageRightGrayGpu, imageLeftGrayGpu;
    cv::cuda::GpuMat keypointsRightGPU, keypointsLeftGPU;
    cv::cuda::GpuMat descriptorsRightGPU, descriptorsLeftGPU;

    imageRightGpu.upload(imageRight);
    imageLeftGpu.upload(imageLeft);

    cuda::cvtColor(imageRightGpu, imageRightGrayGpu, CV_RGB2GRAY);
	cuda::cvtColor(imageLeftGpu, imageLeftGrayGpu, CV_RGB2GRAY);

    surf(imageRightGrayGpu, cv::cuda::GpuMat(), keypointsRightGPU, descriptorsRightGPU);
    surf(imageLeftGrayGpu, cv::cuda::GpuMat(), keypointsLeftGPU, descriptorsLeftGPU);

    cout << "FOUND " << keypointsRightGPU.cols << " keypoints on right image" << endl;
    cout << "FOUND " << keypointsLeftGPU.cols << " keypoints on left image" << endl;

    // matching descriptors
    Ptr<cv::cuda::DescriptorMatcher> matcher = cv::cuda::DescriptorMatcher::createBFMatcher(surf.defaultNorm());
    vector<DMatch> goodMatches;
    vector<vector<DMatch> > matchePts;
    matcher->knnMatch(descriptorsLeftGPU, descriptorsRightGPU, matchePts, 2);

    cout << "matches size:" << matchePts.size() << endl;

    for (int i = 0; i < matchePts.size(); i++)
    {
        if (matchePts[i][0].distance < 0.6 * matchePts[i][1].distance)
        {   
            goodMatches.push_back(matchePts[i][0]);
        }
    }

    cout << "goodMatches size:" << goodMatches.size() << endl;

    // downloading results
    vector<KeyPoint> keypointsRight, keypointsLeft;
    vector<float> descriptors1, descriptors2;
    surf.downloadKeypoints(keypointsRightGPU, keypointsRight);
    surf.downloadKeypoints(keypointsLeftGPU, keypointsLeft);
    // surf.downloadDescriptors(descriptors1GPU, descriptors1);
    // surf.downloadDescriptors(descriptors2GPU, descriptors2);

    vector<Point2f> imagePointsRight, imagePointsLeft;

    for (int i = 0; i<goodMatches.size(); i++)
    {
        imagePointsLeft.push_back(keypointsLeft[goodMatches[i].queryIdx].pt);
        imagePointsRight.push_back(keypointsRight[goodMatches[i].trainIdx].pt);
    }

    homo = findHomography(imagePointsRight, imagePointsLeft, CV_RANSAC);

    cout << "homo：\n" << homo << endl << endl;

    Mat img_matches;
    drawMatches(Mat(imageRightGpu), keypointsRight, Mat(imageLeftGpu), keypointsLeft, matchePts, img_matches);
    cv::imwrite("matches.png", img_matches);

    CalcCorners(homo, imageRight);
    //图像配准  
    Mat imageTransformRight, imageTransformLeft;
    warpPerspective(imageRight, imageTransformRight, homo, Size(MAX(corners.right_top.x, corners.right_bottom.x), imageLeft.rows));
    //warpPerspective(imageRight, imageTransformLeft, adjustMat*homo, Size(imageLeft.cols*1.3, imageLeft.rows*1.8));
    
    //创建拼接后的图,需提前计算图的大小
    int dst_width = imageTransformRight.cols;  //取最右点的长度为拼接图的长度
    int dst_height = imageLeft.rows;

    Mat dst(dst_height, dst_width, CV_8UC3);
    dst.setTo(0);

    imageTransformRight.copyTo(dst(Rect(0, 0, imageTransformRight.cols, imageTransformRight.rows)));
    imageLeft.copyTo(dst(Rect(0, 0, imageLeft.cols, imageLeft.rows)));

    // imwrite("b_dst.jpg", dst);
    OptimizeSeam(imageLeft, imageTransformRight, dst);
    imwrite("dst.jpg", dst);

    // auto startTime= std::chrono::steady_clock::now();

    // stitch(imageRight, imageLeft, ret);

    // auto endTime = std::chrono::steady_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    // cout<<"1 SpendTime = "<<  duration.count() <<"ms"<<endl;

    // startTime= std::chrono::steady_clock::now();

    // for(int i=0;i<30;i++)
    // {
    //     stitch(imageRight, imageLeft, ret);
    // }

    // endTime = std::chrono::steady_clock::now();
    // duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    // cout<<"2 SpendTime = "<<  duration.count()/30 <<"ms"<<endl;


    // imwrite("./dst.jpg", ret);

    return 0;
}