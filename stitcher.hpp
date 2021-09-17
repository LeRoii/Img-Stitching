#ifndef _STITCHER_H_
#define _STITCHER_H_

#include <opencv2/opencv.hpp>
#include <thread>
#include <fstream>
#include <opencv2/stitching.hpp>

#include"gmslcam.hpp"

typedef struct
{
    cv::Point2f left_top;
    cv::Point2f left_bottom;
    cv::Point2f right_top;
    cv::Point2f right_bottom;
}four_corners_t;

four_corners_t corners;

cv::Mat gHomoMat = (cv::Mat_<double>(3,3)<< 0.3463709518587066, -0.02987517114120686, 845.4015037615147, 
                    -0.1465717446443165, 0.9797299227981282, -11.69288605445955,
                    -0.0006891269642287213, 4.812735460740427e-05, 1);

void CalcCorners(const cv::Mat& H, const cv::Mat& src)
{
    std::cout << "H: " << H << std::endl;
    double v2[] = { 0, 0, 1 };//左上角
    double v1[3];//变换后的坐标值
    cv::Mat V2 = cv::Mat(3, 1, CV_64FC1, v2);  //列向量
    cv::Mat V1 = cv::Mat(3, 1, CV_64FC1, v1);  //列向量

    V1 = H * V2;
    //左上角(0,0,1)
    std::cout << "V2: " << V2 << std::endl;
    std::cout << "V1: " << V1 << std::endl;
    corners.left_top.x = v1[0] / v1[2];
    corners.left_top.y = v1[1] / v1[2];

    //左下角(0,src.rows,1)
    v2[0] = 0;
    v2[1] = src.rows;
    v2[2] = 1;
    V2 = cv::Mat(3, 1, CV_64FC1, v2);  //列向量
    V1 = cv::Mat(3, 1, CV_64FC1, v1);  //列向量
    V1 = H * V2;
    corners.left_bottom.x = v1[0] / v1[2];
    corners.left_bottom.y = v1[1] / v1[2];

    //右上角(src.cols,0,1)
    v2[0] = src.cols;
    v2[1] = 0;
    v2[2] = 1;
    V2 = cv::Mat(3, 1, CV_64FC1, v2);  //列向量
    V1 = cv::Mat(3, 1, CV_64FC1, v1);  //列向量
    V1 = H * V2;
    corners.right_top.x = v1[0] / v1[2];
    corners.right_top.y = v1[1] / v1[2];

    std::cout << "V2: " << V2 << std::endl;
    std::cout << "V1: " << V1 << std::endl;

    //右下角(src.cols,src.rows,1)
    v2[0] = src.cols;
    v2[1] = src.rows;
    v2[2] = 1;
    V2 = cv::Mat(3, 1, CV_64FC1, v2);  //列向量
    V1 = cv::Mat(3, 1, CV_64FC1, v1);  //列向量
    V1 = H * V2;
    corners.right_bottom.x = v1[0] / v1[2];
    corners.right_bottom.y = v1[1] / v1[2];

}

void OptimizeSeam(cv::Mat& img1, cv::Mat& trans, cv::Mat& dst)
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

void OptimizeSeamv2(cv::Mat& left, cv::Mat& right, cv::Mat& dst, int offset, int start, int end)
{
    // int start = MIN(corners.left_top.x, corners.left_bottom.x);//开始位置，即重叠区域的左边界  

    double processWidth = left.cols - start;//重叠区域的宽度  
    int rows = left.rows;
    int cols = left.cols; //注意，是列数*通道数
    double alpha = 1;//img1中像素的权重  
    for (int i = 0; i < rows; i++)
    {
        uchar* p = left.ptr<uchar>(i);  //获取第i行的首地址
        uchar* t = right.ptr<uchar>(i);
        uchar* d = dst.ptr<uchar>(i);
        for (int j = start; j < end; j++)
        {
            //如果遇到图像trans中无像素的黑点，则完全拷贝img1中的数据
            if (0)
            {
                alpha = 1;
            }
            else
            {
                //img1中像素的权重，与当前处理点距重叠区域左边界的距离成正比，实验证明，这种方法确实好  
                alpha = (processWidth - (j - start)) / processWidth;
            }

            d[(j+offset) * 3] = p[j * 3] * alpha + t[(j-start) * 3] * (1 - alpha);
            d[(j+offset) * 3 + 1] = p[j * 3 + 1] * alpha + t[(j-start) * 3 + 1] * (1 - alpha);
            d[(j+offset) * 3 + 2] = p[j * 3 + 2] * alpha + t[(j-start) * 3 + 2] * (1 - alpha);

        }
    }

}

struct stitcherCfg
{
    int imgWidth;
    int imgHeight;
};

#define DEBUG 0

cv::Stitcher cvStitcher = cv::Stitcher::createDefault(false);

class stitcher
{
public:
    stitcher(stitcherCfg &cfg, const float scale = 2):m_stCfg(cfg), m_fScale(scale)
    {
        // m_pORB = cv::cuda::ORB::create(500, 1.2f, 6, 31, 0, 2, 0, 31, 20,true);
        m_pORB = cv::cuda::ORB::create(500, 1.2f, 6, 31, 0, 2, 0, 31, 20,true);
        m_pORB = cv::cuda::ORB::create(1000);
        // m_pMatcher = cv::cuda::DescriptorMatcher::createBFMatcher(cv::NORM_L2);

        m_pSURF = new cv::cuda::SURF_CUDA(400);
        m_pMatcher = cv::cuda::DescriptorMatcher::createBFMatcher(m_pSURF->defaultNorm());

        m_stCfg.imgWidth = m_stCfg.imgWidth/m_fScale;
        m_stCfg.imgHeight = m_stCfg.imgHeight/m_fScale;
        int maskWidth = m_stCfg.imgWidth/4;

        // construct mask
        cv::Mat mask = cv::Mat::zeros(cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight), CV_8UC1);
        mask(cv::Rect(0, 0, maskWidth, m_stCfg.imgHeight)).setTo(255);
        m_gmMaskR = cv::cuda::GpuMat(mask);
        mask.setTo(0);
        mask(cv::Rect(m_stCfg.imgWidth - maskWidth, 0, maskWidth, m_stCfg.imgHeight)).setTo(255);
        m_gmMaskL = cv::cuda::GpuMat(mask);

        // m_gmDst = cv::cuda::GpuMat(cv::Mat::zeros(1080, 5240, CV_8UC3));
        // dst = cv::Mat::zeros(1080, 5240, CV_8UC3);
        // tr = cv::Mat::zeros(1080, 5240, CV_8UC3);

        std::ifstream fin("Homography.txt", std::ios::in);
        if(fin.is_open())
        {
            std::string a;
            double homoMat[9];
            m_homo = cv::Mat(cv::Size(3, 3), CV_64FC1);
            double *ptr = (double*)m_homo.data;
            for(int i=0;i<9;i++)
            {
                std::string str;
                fin >> str;
                if(i==0)
                {
                    str = str.substr(1);
                }
                str.pop_back();
                // std::cout << str << std::endl;
                *(ptr+i) = stod(str);
            }
            
            // m_homo = gHomoMat;
            std::cout << m_homo << std::endl;
        }

    }

    ~stitcher()
    {
        if(m_pSURF != nullptr)
        {
            delete m_pSURF;
            m_pSURF = nullptr;
        }
    }

    void dete(int imgid)
    {
        if(imgid == 0)  //left
        {
           (*m_pSURF)(m_gmImgGrayL, m_gmMaskL, m_gmKeyPtsL, m_gmDescriptorL); 
        }
        else if(imgid)
        {
            (*m_pSURF)(m_gmImgGrayR, m_gmMaskR, m_gmKeyPtsR, m_gmDescriptorR);
        }
        printf("det imgid:%d\n", imgid);
    }

    void process(cv::Mat &imgRight, cv::Mat &imgLeft, cv::Mat &ret)
    {
        auto startTime= std::chrono::steady_clock::now();

        cv::resize(imgRight, imgRight, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));
        cv::resize(imgLeft, imgLeft, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));

        m_gmImgR.upload(imgRight);
        m_gmImgL.upload(imgLeft);

        // cv::cuda::resize(m_gmImgR, m_gmImgR, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));
        // cv::cuda::resize(m_gmImgL, m_gmImgL, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));
        cv::cuda::cvtColor(m_gmImgR, m_gmImgGrayR, CV_RGB2GRAY);
	    cv::cuda::cvtColor(m_gmImgL, m_gmImgGrayL, CV_RGB2GRAY);

        (*m_pSURF)(m_gmImgGrayR, m_gmMaskR, m_gmKeyPtsR, m_gmDescriptorR);
        (*m_pSURF)(m_gmImgGrayL, m_gmMaskL, m_gmKeyPtsL, m_gmDescriptorL);

        // std::thread th1(&stitcher::dete, this, 0);
        // std::thread th2(&stitcher::dete, this, 1);
        // // th1.detach();
        // // th2.detach();
        // th1.join();
        // th2.join();
#if DEBUG
        std::cout << "FOUND " << m_gmKeyPtsL.cols << " keypoints on right image" << std::endl;
        std::cout << "FOUND " << m_gmKeyPtsL.cols << " keypoints on left image" << std::endl;
#endif
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout<<"feature det SpendTime = "<<  duration.count() <<"ms"<<std::endl;

        startTime= std::chrono::steady_clock::now();

        std::vector<cv::DMatch> goodMatches;
        std::vector<std::vector<cv::DMatch> > matchePts;
        m_pMatcher->knnMatch(m_gmDescriptorL, m_gmDescriptorR, matchePts, 2);

        // std::cout << "matches size:" << matchePts.size() << std::endl;

        for (int i = 0; i < matchePts.size(); i++)
        {
            if (matchePts[i][0].distance < 0.6 * matchePts[i][1].distance)
            {   
                goodMatches.push_back(matchePts[i][0]);
            }
        }
        
        // downloading results
        std::vector<cv::KeyPoint> keypointsRight, keypointsLeft;
        std::vector<float> descriptors1, descriptors2;
        m_pSURF->downloadKeypoints(m_gmKeyPtsR, keypointsRight);
        m_pSURF->downloadKeypoints(m_gmKeyPtsL, keypointsLeft);
        // surf.downloadDescriptors(descriptors1GPU, descriptors1);
        // surf.downloadDescriptors(descriptors2GPU, descriptors2);

        std::vector<cv::Point2f> imagePointsRight, imagePointsLeft;

        for (int i = 0; i<goodMatches.size(); i++)
        {
            imagePointsLeft.push_back(keypointsLeft[goodMatches[i].queryIdx].pt);
            imagePointsRight.push_back(keypointsRight[goodMatches[i].trainIdx].pt);
        }
#if DEBUG
        std::cout << "goodMatches size:" << goodMatches.size() << std::endl;
        cv::Mat imgleftkeypts;
        cv::drawKeypoints(imgLeft, keypointsLeft, imgleftkeypts);
        cv::imwrite("imgleftkeypts.png ", imgleftkeypts);

        cv::Mat first_match;
        cv::drawMatches(imgLeft, keypointsLeft, imgRight, keypointsRight, goodMatches, first_match);
        cv::imwrite("first_match.png ", first_match);
#endif
        cv::Mat homo = findHomography(imagePointsRight, imagePointsLeft, CV_RANSAC);
        

        CalcCorners(homo, imgRight);
        //图像配准  
        cv::Mat imageTransformRight;
        cv::warpPerspective(imgRight, imageTransformRight, homo, cv::Size(MAX(corners.right_top.x, corners.right_bottom.x), imgLeft.rows));
        //warpPerspective(imageRight, imageTransformLeft, adjustMat*homo, Size(imageLeft.cols*1.3, imageLeft.rows*1.8));
#if DEBUG
        std::cout << "homo：\n" << homo << std::endl;
        cv::imwrite("imageTransformRight.png", imageTransformRight);
#endif
        // cv::cuda::GpuMat imageTransformRightGpu;
        // cv::cuda::warpPerspective(m_gmImgR, imageTransformRightGpu, homo, cv::Size(MAX(corners.right_top.x, corners.right_bottom.x), imgLeft.rows));
        // cv::Mat imageTransformRight(imageTransformRightGpu);
        //创建拼接后的图,需提前计算图的大小
        int dst_width = imageTransformRight.cols;  //取最右点的长度为拼接图的长度
        int dst_height = imgLeft.rows;

        cv::Mat dst(dst_height, dst_width, CV_8UC3);
        dst.setTo(0);

        imageTransformRight.copyTo(dst(cv::Rect(0, 0, imageTransformRight.cols, imageTransformRight.rows)));
        imgLeft.copyTo(dst(cv::Rect(0, 0, imgLeft.cols, imgLeft.rows)));

        // imwrite("b_dst.jpg", dst);
        OptimizeSeam(imgLeft, imageTransformRight, dst);
#if DEBUG
        cv::imwrite("dst.jpg", dst);
#endif
        endTime = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout<<"frame SpendTime = "<<  duration.count() <<"ms"<<std::endl;

    }

    void processorb(cv::Mat &imgRight, cv::Mat &imgLeft, cv::Mat &ret)
    {
        auto startTime= std::chrono::steady_clock::now();

        cv::resize(imgRight, imgRight, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));
        cv::resize(imgLeft, imgLeft, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));

        m_gmImgR.upload(imgRight);
        m_gmImgL.upload(imgLeft);

        // cv::cuda::resize(m_gmImgR, m_gmImgR, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));
        // cv::cuda::resize(m_gmImgL, m_gmImgL, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));
        cv::cuda::cvtColor(m_gmImgR, m_gmImgGrayR, CV_RGB2GRAY);
	    cv::cuda::cvtColor(m_gmImgL, m_gmImgGrayL, CV_RGB2GRAY);

        m_pORB->detectAndComputeAsync(m_gmImgGrayR, m_gmMaskR, m_gmKeyPtsR, m_gmDescriptorR);
        m_pORB->detectAndComputeAsync(m_gmImgGrayL, m_gmMaskL, m_gmKeyPtsL, m_gmDescriptorL);

#if DEBUG
        std::cout << "FOUND " << m_gmKeyPtsL.cols << " keypoints on right image" << std::endl;
        std::cout << "FOUND " << m_gmKeyPtsL.cols << " keypoints on left image" << std::endl;
#endif
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout<<"feature det SpendTime = "<<  duration.count() <<"ms"<<std::endl;
        startTime= std::chrono::steady_clock::now();

        // downloading results
        std::vector<cv::KeyPoint> keypointsRight, keypointsLeft;
        std::vector<float> descriptors1, descriptors2;
        m_pORB->convert(m_gmKeyPtsR, keypointsRight);
        m_pORB->convert(m_gmKeyPtsL, keypointsLeft);
        m_gmDescriptorR.convertTo(m_gmDescriptorR32, CV_32F);
        m_gmDescriptorL.convertTo(m_gmDescriptorL32, CV_32F);

        std::vector<cv::DMatch> goodMatches;
        std::vector<cv::DMatch> matches;
        std::vector<std::vector<cv::DMatch> > matchePts;
        m_pMatcher->match(m_gmDescriptorL32, m_gmDescriptorR32, matches);

        int sz = matches.size();
		double max_dist = 0; double min_dist = 100;
        for (int i = 0; i < sz; i++)
		{
			double dist = matches[i].distance;
			if (dist < min_dist) min_dist = dist;
			if (dist > max_dist) max_dist = dist;
		}
 #if DEBUG
		std::cout << "\n-- Max dist : " << max_dist << std::endl;
		std::cout << "\n-- Min dist : " << min_dist << std::endl;
#endif
 
		for (int i = 0; i < sz; i++)
		{
			if (matches[i].distance < 0.6*max_dist)
			{
				goodMatches.push_back(matches[i]);
			}
		}

        // std::cout << "matches size:" << matchePts.size() << std::endl;

        // m_pSURF->downloadKeypoints(m_gmKeyPtsR, keypointsRight);
        // m_pSURF->downloadKeypoints(m_gmKeyPtsL, keypointsLeft);
        // surf.downloadDescriptors(descriptors1GPU, descriptors1);
        // surf.downloadDescriptors(descriptors2GPU, descriptors2);

        std::vector<cv::Point2f> imagePointsRight, imagePointsLeft;

        for (int i = 0; i<goodMatches.size(); i++)
        {
            imagePointsLeft.push_back(keypointsLeft[goodMatches[i].queryIdx].pt);
            imagePointsRight.push_back(keypointsRight[goodMatches[i].trainIdx].pt);
        }
#if DEBUG
        std::cout << "goodMatches size:" << goodMatches.size() << std::endl;
        cv::Mat imgleftkeypts;
        cv::drawKeypoints(imgLeft, keypointsLeft, imgleftkeypts);
        cv::imwrite("imgleftkeypts.png ", imgleftkeypts);

        cv::Mat first_match;
        cv::drawMatches(imgLeft, keypointsLeft, imgRight, keypointsRight, goodMatches, first_match);
        cv::imwrite("first_match.png ", first_match);
#endif
        cv::Mat homo = findHomography(imagePointsRight, imagePointsLeft, CV_RANSAC);
        // std::cout << "homo：\n" << homo << std::endl;

        CalcCorners(homo, imgRight);
        //图像配准  
        cv::Mat imageTransformRight;
        cv::warpPerspective(imgRight, imageTransformRight, homo, cv::Size(MAX(corners.right_top.x, corners.right_bottom.x), imgLeft.rows));
        //warpPerspective(imageRight, imageTransformLeft, adjustMat*homo, Size(imageLeft.cols*1.3, imageLeft.rows*1.8));
#if DEBUG
        cv::imwrite("imageTransformRight.png", imageTransformRight);
#endif
        // cv::cuda::GpuMat imageTransformRightGpu;
        // cv::cuda::warpPerspective(m_gmImgR, imageTransformRightGpu, homo, cv::Size(MAX(corners.right_top.x, corners.right_bottom.x), imgLeft.rows));
        // cv::Mat imageTransformRight(imageTransformRightGpu);
        //创建拼接后的图,需提前计算图的大小
        int dst_width = imageTransformRight.cols;  //取最右点的长度为拼接图的长度
        int dst_height = imgLeft.rows;

        cv::Mat dst(dst_height, dst_width, CV_8UC3);
        dst.setTo(0);

        imageTransformRight.copyTo(dst(cv::Rect(0, 0, imageTransformRight.cols, imageTransformRight.rows)));
        imgLeft.copyTo(dst(cv::Rect(0, 0, imgLeft.cols, imgLeft.rows)));

        // imwrite("b_dst.jpg", dst);
        OptimizeSeam(imgLeft, imageTransformRight, dst);
#if DEBUG
        cv::imwrite("dst.jpg", dst);
#endif
        endTime = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout<<"frame SpendTime = "<<  duration.count() <<"ms"<<std::endl;

    }

    void processnodet(cv::Mat &imgRight, cv::Mat &imgLeft, cv::Mat &ret)
    {
        auto startTime= std::chrono::steady_clock::now();

        if(m_fScale != 1)
        {
            cv::resize(imgRight, imgRight, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));
            cv::resize(imgLeft, imgLeft, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));
        }

        m_gmImgR.upload(imgRight);
        m_gmImgL.upload(imgLeft);

        // cv::cuda::resize(m_gmImgR, m_gmImgR, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));
        // cv::cuda::resize(m_gmImgL, m_gmImgL, cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight));
        cv::cuda::cvtColor(m_gmImgR, m_gmImgGrayR, CV_RGB2GRAY);
	    cv::cuda::cvtColor(m_gmImgL, m_gmImgGrayL, CV_RGB2GRAY);

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout<<"upload SpendTime = "<<  duration.count() <<"ms"<<std::endl;

        startTime= std::chrono::steady_clock::now();
        CalcCorners(m_homo, imgRight);
#if DEBUG
        std::cout << "left_top:" << corners.left_top << std::endl;
        std::cout << "left_bottom:" << corners.left_bottom << std::endl;
        std::cout << "right_top:" << corners.right_top << std::endl;
        std::cout << "right_bottom:" << corners.right_bottom << std::endl;
#endif
        //图像配准  
        // cv::Mat imageTransformRight;
        // cv::warpPerspective(imgRight, imageTransformRight, homoMat, cv::Size(MAX(corners.right_top.x, corners.right_bottom.x), imgLeft.rows));
        //warpPerspective(imageRight, imageTransformLeft, adjustMat*homo, Size(imageLeft.cols*1.3, imageLeft.rows*1.8));
 
        // cv::cuda::warpPerspective(m_gmImgR, m_gmImgTransformRight, m_homo, cv::Size(MAX(corners.right_top.x, corners.right_bottom.x), imgLeft.rows));
        cv::cuda::warpPerspective(m_gmImgR, m_gmImgTransformRight, m_homo, cv::Size(imgLeft.cols*2, imgLeft.rows));

#if DEBUG
        std::cout << "homo：\n" << m_homo << std::endl;
        cv::imwrite("imageTransformRight.png", cv::Mat(m_gmImgTransformRight));
#endif

        int dst_width = m_gmImgTransformRight.cols;  //取最右点的长度为拼接图的长度
        int dst_height = imgLeft.rows;
        m_gmDst = cv::cuda::GpuMat(cv::Mat::zeros(dst_height, dst_width, CV_8UC3));
        
        m_gmImgTransformRight.copyTo(m_gmDst(cv::Rect(0, 0, m_gmImgTransformRight.cols, m_gmImgTransformRight.rows)));
        m_gmImgL.copyTo(m_gmDst(cv::Rect(0, 0, m_gmImgL.cols, m_gmImgL.rows)));
        
        // cv::Mat tr;
        m_gmImgTransformRight.download(tr);
        m_gmDst.download(ret);
#if DEBUG      
        cv::imwrite("b_dst.jpg", ret);
#endif
        OptimizeSeam(imgLeft, tr, ret);
#if DEBUG
        cv::imwrite("dst.jpg", ret);
#endif
        endTime = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout<<"OptimizeSeam SpendTime = "<<  duration.count() <<"ms"<<std::endl;
         
    }

    void processall(gmslCamera cams[], int num, cv::Mat &final)
    {
        auto startTime= std::chrono::steady_clock::now();
        // cv::Mat ret = cv::Mat(2160, 3840, CV_8UC3);
        cv::Mat ret = cv::Mat(1080, 3840, CV_8UC3);
        // cv::imshow("m_dev_name", cams[2].m_ret);
        // cv::waitKey(33);

        int seam12 = 700;
        int seam01 = 695;
        cams[2].m_ret.copyTo(ret(cv::Rect(0,0, 960, 540)));
        cams[1].m_ret.copyTo(ret(cv::Rect(seam12, 0, 960, 540)));
        OptimizeSeamv2(cams[2].m_ret, cams[1].m_ret, ret, 0, seam12, 960);
        cams[0].m_ret.copyTo(ret(cv::Rect(seam12+seam01, 0, 960, 540)));
        OptimizeSeamv2(cams[1].m_ret, cams[0].m_ret, ret, seam12, seam01, 960);
        final=ret;
        // cv::resize(ret, final, cv::Size(1920,1080));

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        std::cout<<"processall SpendTime = "<<  duration.count() <<"ms"<<std::endl;

    }

    void stitcherProcess(gmslCamera cams[], int num, cv::Mat &final)
    {
        cv::Mat pano;
        std::vector<cv::Mat> imgs;
        for(int i=0;i<num;i++)
            imgs.push_back(cams[i].m_ret);

        // cv::Stitcher cvStitcher = cv::Stitcher::createDefault(false);
        cv::Stitcher::Status status = cvStitcher.stitch(imgs, final);

        if (status != cv::Stitcher::OK) 
        {   
            std::cout << "Can't stitch images, error code = " << int(status) << std::endl;   
            return;   
        }   
    }

private:
    stitcherCfg m_stCfg;
    float m_fScale;
    cv::cuda::SURF_CUDA *m_pSURF;
    cv::Ptr<cv::cuda::ORB> m_pORB;
    cv::Ptr<cv::cuda::DescriptorMatcher> m_pMatcher;
    cv::cuda::GpuMat m_gmImgR, m_gmImgL;
    cv::cuda::GpuMat m_gmMaskR, m_gmMaskL;
    cv::cuda::GpuMat m_gmImgGrayR, m_gmImgGrayL;
    cv::cuda::GpuMat m_gmKeyPtsR, m_gmKeyPtsL;
    cv::cuda::GpuMat m_gmDescriptorR, m_gmDescriptorL;
    cv::cuda::GpuMat m_gmDescriptorR32, m_gmDescriptorL32;
    cv::cuda::GpuMat m_gmDst, m_gmImgTransformRight;
    cv::Mat tr, dst, m_homo;
    // cv::Stitcher *cvStitcher;
};

#endif