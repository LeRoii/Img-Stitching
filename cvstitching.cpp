#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <iostream>
#include <chrono> 

using namespace cv;
using namespace std;

void OptimizeSeam(Mat& img1, Mat& trans, Mat& dst);

typedef struct
{
    Point2f left_top;
    Point2f left_bottom;
    Point2f right_top;
    Point2f right_bottom;
}four_corners_t;

four_corners_t corners;

void CalcCorners(const Mat& H, const Mat& src)
{
    double v2[] = { 0, 0, 1 };//左上角
    double v1[3];//变换后的坐标值
    Mat V2 = Mat(3, 1, CV_64FC1, v2);  //列向量
    Mat V1 = Mat(3, 1, CV_64FC1, v1);  //列向量

    V1 = H * V2;
    //左上角(0,0,1)
    cout << "V2: " << V2 << endl;
    cout << "V1: " << V1 << endl;
    corners.left_top.x = v1[0] / v1[2];
    corners.left_top.y = v1[1] / v1[2];

    //左下角(0,src.rows,1)
    v2[0] = 0;
    v2[1] = src.rows;
    v2[2] = 1;
    V2 = Mat(3, 1, CV_64FC1, v2);  //列向量
    V1 = Mat(3, 1, CV_64FC1, v1);  //列向量
    V1 = H * V2;
    corners.left_bottom.x = v1[0] / v1[2];
    corners.left_bottom.y = v1[1] / v1[2];

    //右上角(src.cols,0,1)
    v2[0] = src.cols;
    v2[1] = 0;
    v2[2] = 1;
    V2 = Mat(3, 1, CV_64FC1, v2);  //列向量
    V1 = Mat(3, 1, CV_64FC1, v1);  //列向量
    V1 = H * V2;
    corners.right_top.x = v1[0] / v1[2];
    corners.right_top.y = v1[1] / v1[2];

    //右下角(src.cols,src.rows,1)
    v2[0] = src.cols;
    v2[1] = src.rows;
    v2[2] = 1;
    V2 = Mat(3, 1, CV_64FC1, v2);  //列向量
    V1 = Mat(3, 1, CV_64FC1, v1);  //列向量
    V1 = H * V2;
    corners.right_bottom.x = v1[0] / v1[2];
    corners.right_bottom.y = v1[1] / v1[2];

}

int main(int argc, char *argv[])
{
    Mat imageRight = imread("../imgs/01.png", 1);    //右图
    Mat imageLeft = imread("../imgs/02.png", 1);    //左图

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


//优化两图的连接处，使得拼接自然
void OptimizeSeam(Mat& img1, Mat& trans, Mat& dst)
{
    int start = MIN(corners.left_top.x, corners.left_bottom.x);//开始位置，即重叠区域的左边界  

    double processWidth = img1.cols - start;//重叠区域的宽度  
    int rows = dst.rows;
    int cols = img1.cols; //注意，是列数*通道数
    double alpha = 1;//img1中像素的权重  
    for (int i = 0; i < rows; i++)
    {
        uchar* p = img1.ptr<uchar>(i);  //获取第i行的首地址
        uchar* t = trans.ptr<uchar>(i);
        uchar* d = dst.ptr<uchar>(i);
        for (int j = start; j < cols; j++)
        {
            //如果遇到图像trans中无像素的黑点，则完全拷贝img1中的数据
            if (t[j * 3] == 0 && t[j * 3 + 1] == 0 && t[j * 3 + 2] == 0)
            {
                alpha = 1;
            }
            else
            {
                //img1中像素的权重，与当前处理点距重叠区域左边界的距离成正比，实验证明，这种方法确实好  
                alpha = (processWidth - (j - start)) / processWidth;
            }

            d[j * 3] = p[j * 3] * alpha + t[j * 3] * (1 - alpha);
            d[j * 3 + 1] = p[j * 3 + 1] * alpha + t[j * 3 + 1] * (1 - alpha);
            d[j * 3 + 2] = p[j * 3 + 2] * alpha + t[j * 3 + 2] * (1 - alpha);

        }
    }

}