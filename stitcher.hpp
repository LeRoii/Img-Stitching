#include <opencv2/opencv.hpp>

typedef struct
{
    cv::Point2f left_top;
    cv::Point2f left_bottom;
    cv::Point2f right_top;
    cv::Point2f right_bottom;
}four_corners_t;

four_corners_t corners;

void CalcCorners(const cv::Mat& H, const cv::Mat& src)
{
    double v2[] = { 0, 0, 1 };//左上角
    double v1[3];//变换后的坐标值
    cv::Mat V2 = cv::Mat(3, 1, CV_64FC1, v2);  //列向量
    cv::Mat V1 = cv::Mat(3, 1, CV_64FC1, v1);  //列向量

    V1 = H * V2;
    //左上角(0,0,1)
    // cout << "V2: " << V2 << endl;
    // cout << "V1: " << V1 << endl;
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

struct stitcherCfg
{
    int imgWidth;
    int imgHeight;
};

class stitcher
{
public:
    stitcher(stitcherCfg &cfg, const float scale = 2):m_stCfg(cfg), m_fScale(scale)
    {
        m_pSURF = new cv::cuda::SURF_CUDA(400);
        m_pMatcher = cv::cuda::DescriptorMatcher::createBFMatcher(m_pSURF->defaultNorm());

        // construct mask
        cv::Mat mask = cv::Mat::zeros(cv::Size(m_stCfg.imgWidth, m_stCfg.imgHeight), CV_8UC1);
        int maskWidth = m_stCfg.imgWidth/2;
        mask(cv::Rect(0, 0, maskWidth, m_stCfg.imgHeight)).setTo(255);
        m_gmMaskR = cv::cuda::GpuMat(mask);
        mask.setTo(0);
        mask(cv::Rect(maskWidth, 0, maskWidth, m_stCfg.imgHeight)).setTo(255);
        m_gmMaskL = cv::cuda::GpuMat(mask);
    }

    ~stitcher()
    {
        if(m_pSURF != nullptr)
        {
            delete m_pSURF;
            m_pSURF = nullptr;
        }
    }
    
    void process(cv::Mat &imgRight, cv::Mat &imgLeft, cv::Mat &ret)
    {
        m_gmImgR.upload(imgRight);
        m_gmImgL.upload(imgLeft);

        cv::cuda::cvtColor(m_gmImgR, m_gmImgGrayR, CV_RGB2GRAY);
	    cv::cuda::cvtColor(m_gmImgL, m_gmImgGrayL, CV_RGB2GRAY);

        (*m_pSURF)(m_gmImgGrayR, m_gmMaskR, m_gmKeyPtsR, m_gmDescriptorR);
        (*m_pSURF)(m_gmImgGrayL, m_gmMaskL, m_gmKeyPtsL, m_gmDescriptorL);

        std::cout << "FOUND " << m_gmKeyPtsL.cols << " keypoints on right image" << std::endl;
        std::cout << "FOUND " << m_gmKeyPtsL.cols << " keypoints on left image" << std::endl;

        std::vector<cv::DMatch> goodMatches;
        std::vector<std::vector<cv::DMatch> > matchePts;
        m_pMatcher->knnMatch(m_gmDescriptorL, m_gmDescriptorR, matchePts, 2);

        std::cout << "matches size:" << matchePts.size() << std::endl;

        for (int i = 0; i < matchePts.size(); i++)
        {
            if (matchePts[i][0].distance < 0.6 * matchePts[i][1].distance)
            {   
                goodMatches.push_back(matchePts[i][0]);
            }
        }

        std::cout << "goodMatches size:" << goodMatches.size() << std::endl;

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

        cv::Mat homo = findHomography(imagePointsRight, imagePointsLeft, CV_RANSAC);

        // std::cout << "homo：\n" << homo << std::endl;

        cv::Mat imgleftkeypts;
        cv::drawKeypoints(imgLeft, keypointsLeft, imgleftkeypts);
        // cv::imwrite("imgleftkeypts.png ", imgleftkeypts);

        cv::Mat first_match;
        cv::drawMatches(imgLeft, keypointsLeft, imgRight, keypointsRight, goodMatches, first_match);
        // cv::imwrite("first_match.png ", first_match);

        CalcCorners(homo, imgRight);
        //图像配准  
        cv::Mat imageTransformRight;
        cv::warpPerspective(imgRight, imageTransformRight, homo, cv::Size(MAX(corners.right_top.x, corners.right_bottom.x), imgLeft.rows));
        //warpPerspective(imageRight, imageTransformLeft, adjustMat*homo, Size(imageLeft.cols*1.3, imageLeft.rows*1.8));

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
        cv::imwrite("dst.jpg", dst);
    }

private:
    stitcherCfg m_stCfg;
    float m_fScale;
    cv::cuda::SURF_CUDA *m_pSURF;
    cv::Ptr<cv::cuda::DescriptorMatcher> m_pMatcher;
    cv::cuda::GpuMat m_gmImgR, m_gmImgL;
    cv::cuda::GpuMat m_gmMaskR, m_gmMaskL;
    cv::cuda::GpuMat m_gmImgGrayR, m_gmImgGrayL;
    cv::cuda::GpuMat m_gmKeyPtsR, m_gmKeyPtsL;
    cv::cuda::GpuMat m_gmDescriptorR, m_gmDescriptorL;
};