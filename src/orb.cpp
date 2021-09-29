#include <iostream>
#include <signal.h>
 
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/cudabgsegm.hpp"
#include "opencv2/core/cuda.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/cuda_stream_accessor.hpp"
#include "opencv2/cudafeatures2d.hpp"
 
#include "opencv2/cudaimgproc.hpp"
#include "opencv2/cudaarithm.hpp"
#include "opencv2/cudafilters.hpp"
#include "opencv2/cudawarping.hpp"
 
#include "opencv2/features2d.hpp"
#include <vector>
 
using namespace cv;
using namespace cuda;
using namespace std;
 
 
bool stop = false;
void sigIntHandler(int signal)
{
    stop = true;
    cout<<"Honestly, you are out!"<<endl;
}
 
 
int main()
{
	Mat img_1 = imread("model.jpg");
	Mat img_2 = imread("ORB_test.jpg");
 
	if (!img_1.data || !img_2.data)
	{
		cout << "error reading images " << endl;
		return -1;
	}
 
	int times = 0;
	double startime = cv::getTickCount();
	signal(SIGINT, sigIntHandler);
 
	int64 start, end;
	double time;
 
	vector<Point2f> recognized;
	vector<Point2f> scene;
 
	for(times = 0;!stop; times++)
	{
		start = getTickCount();
 
		recognized.resize(500);
		scene.resize(500);
 
		cuda::GpuMat d_img1, d_img2;
		cuda::GpuMat d_srcL, d_srcR;
 
		d_img1.upload(img_1); d_img2.upload(img_2);
 
		Mat img_matches, des_L, des_R;
 
		cuda::cvtColor(d_img1, d_srcL, COLOR_BGR2GRAY);
		cuda::cvtColor(d_img2, d_srcR, COLOR_BGR2GRAY);
 
		Ptr<cuda::ORB> d_orb = cuda::ORB::create(500, 1.2f, 6, 31, 0, 2, 0, 31, 20,true);
 
		cuda::GpuMat d_keypointsL, d_keypointsR;
		cuda::GpuMat d_descriptorsL, d_descriptorsR, d_descriptorsL_32F, d_descriptorsR_32F;
 
		vector<KeyPoint> keyPoints_1, keyPoints_2;
 
		Ptr<cv::cuda::DescriptorMatcher> d_matcher = cv::cuda::DescriptorMatcher::createBFMatcher(NORM_L2);
 
		std::vector<DMatch> matches;
		std::vector<DMatch> good_matches;
 
		d_orb -> detectAndComputeAsync(d_srcL, cuda::GpuMat(), d_keypointsL, d_descriptorsL);
		d_orb -> convert(d_keypointsL, keyPoints_1);
		d_descriptorsL.convertTo(d_descriptorsL_32F, CV_32F);
 
		d_orb -> detectAndComputeAsync(d_srcR, cuda::GpuMat(), d_keypointsR, d_descriptorsR);
		d_orb -> convert(d_keypointsR, keyPoints_2);
		d_descriptorsR.convertTo(d_descriptorsR_32F, CV_32F);
 
		d_matcher -> match(d_descriptorsL_32F, d_descriptorsR_32F, matches);
 
		int sz = matches.size();
		double max_dist = 0; double min_dist = 100;
 
		for (int i = 0; i < sz; i++)
		{
			double dist = matches[i].distance;
			if (dist < min_dist) min_dist = dist;
			if (dist > max_dist) max_dist = dist;
		}
 
		cout << "\n-- Max dist : " << max_dist << endl;
		cout << "\n-- Min dist : " << min_dist << endl;
 
		for (int i = 0; i < sz; i++)
		{
			if (matches[i].distance < 0.6*max_dist)
			{
				good_matches.push_back(matches[i]);
			}
		}
 
		for (size_t i = 0; i < good_matches.size(); ++i)
		{
			scene.push_back(keyPoints_2[ good_matches[i].trainIdx ].pt);
		}
 
		for(unsigned int j = 0; j < scene.size(); j++)
			cv::circle(img_2, scene[j], 2, cv::Scalar(0, 255, 0), 2);
 
		//imshow("img_2", img_2);
		//waitKey(1);
 
		end = getTickCount();
		time = (double)(end - start) * 1000 / getTickFrequency();
		cout << "Total time : " << time << " ms"<<endl;
 
		if (times == 1000)
		{
			double maxvalue =  (cv::getTickCount() - startime)/cv::getTickFrequency();
			cout <<"zhenshu " << times/maxvalue <<"  zhen"<<endl;
		}
		cout <<"The number of frame is :  " <<times<<endl;
	}
 
	return 0;
}
 