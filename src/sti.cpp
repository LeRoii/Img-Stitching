#include <iostream>  
#include <fstream>  
#include <string>  
#include "opencv2/opencv_modules.hpp"  
#include "opencv2/highgui/highgui.hpp"  
#include <opencv2/stitching.hpp>
using namespace std;  
using namespace cv;  
// using namespace cv::detail;  
 
int main( ) 
{   
   vector<Mat> imgs;    //输入9幅图像
   Mat img;
   img = imread("1-dist.png");	
   imgs.push_back(img);
   img = imread("2-dist.png");	
   imgs.push_back(img);
   // img = imread("3-dist.png");	
   // imgs.push_back(img);
   // img = imread("4-dist.png");	
   // imgs.push_back(img);
   // img = imread("5-dist.png");	
   // imgs.push_back(img);
   // img = imread("6-dist.png");	
   // imgs.push_back(img);

   // vector<Mat> masks;
   // cv::Mat mask = cv::Mat::zeros(cv::Size(960,540), CV_8UC1);
   // cv::Mat mask1 = mask(Rect(0,0,480,540)).setTo(255).clone();
   // mask.setTo(0);
   // cv::Mat mask2 = mask.setTo(255).clone();
   // cv::Mat mask3 = mask(Rect(480,0,480,540)).setTo(255).clone();
   // masks.push_back(mask1);
   // masks.push_back(mask2);
   // masks.push_back(mask3);

   vector<vector<Rect>> roi;
   vector<Rect> roi1 = {Rect{0,0,480,540}};
   vector<Rect> roi2 = {Rect{0,0,960,540}};
   vector<Rect> roi3 = {Rect{480,0,480,540}};
   roi.push_back(roi1);
   roi.push_back(roi2);
   roi.push_back(roi3);
   

//    img = imread("7.jpg");	
//    imgs.push_back(img);
//    img = imread("8.jpg");	
//    imgs.push_back(img);
//    img = imread("9.jpg");	
//    imgs.push_back(img);	
//    img = imread("10.jpg");	
//    imgs.push_back(img);
//    img = imread("11.jpg");	
//    imgs.push_back(img);
//    img = imread("12.jpg");	
//    imgs.push_back(img);
 
   int num_images = 2;
   
   Mat pano;    //全景图像
   Stitcher stitcher = Stitcher::createDefault(false);    //定义全景图像拼接器
   Stitcher::Status status = stitcher.stitch(imgs, pano);    //图像拼接
    
   if (status != Stitcher::OK) 
   {   
      cout << "Can't stitch images, error code = " << int(status) << endl;   
      return -1;   
   }   
   
   imwrite("pano.jpg", pano);    //存储图像
   return 0;   

   // Mat m1 = imread("1-4.jpg");
   // Mat m2 = imread("5-8.jpg");
   // cv::resize(m2, m2, cv::Size(2506,564));
   // Mat ret = Mat::zeros(cv::Size(2506, 564*2+10), CV_8UC3);

   // m1.copyTo(ret(Rect(0,0,2506,564)));
   // m2.copyTo(ret(Rect(0,574,2506,564)));

   // imwrite("ffff.png", ret);


}