#include <opencv2/opencv.hpp>
#include <thread>
#include "stitcher.hpp"
#include <string>

#define USED_CAMERA_NUM 6

int main(int argc, char *argv[])
{
    stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{3840,2160,1920/2,1080/2,1920/2,1080/2,1,"/dev/video0"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/2,1080/2,2,"/dev/video1"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/2,1080/2,3,"/dev/video2"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/2,1080/2,4,"/dev/video3"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/2,1080/2,5,"/dev/video4"},
                                    stCamCfg{3840,2160,1920/2,1080/2,1920/2,1080/2,6,"/dev/video5"}};
    gmslCamera cameras[CAMERA_NUM] = {gmslCamera{camcfgs[0]},
                                    gmslCamera{camcfgs[1]},
                                    gmslCamera{camcfgs[2]},
                                    gmslCamera{camcfgs[3]},
                                    gmslCamera{camcfgs[4]},
                                    gmslCamera{camcfgs[5]}};
    cv::Mat ret;// = cv::Mat(2160, 3840, CV_8UC3);

    stitcherCfg stitchercfg;
    stitchercfg.imgHeight = camcfgs[0].retHeight;
    stitchercfg.imgWidth = camcfgs[0].retWidth;

    stitcher imgstither(stitchercfg, 1);

    cv::Ptr<cv::Stitcher> stitcher = cv::Stitcher::create(cv::Stitcher::PANORAMA, true);
    while(1)
    {
        // cam0.read_frame();
        // cam1.read_frame();
        // cameras[0].read_frame();
		std::vector<std::thread> threads;
		for(int i=0;i<USED_CAMERA_NUM;i++)
			threads.push_back(std::thread(&gmslCamera::read_frame, std::ref(cameras[i])));
		for(auto& th:threads)
			th.join();

        // std::thread th1(&gmslCamera::read_frame, std::ref(cameras[0]));
        // th1.join();
        // std::thread th2(&gmslCamera::read_frame, std::ref(cameras[1]));
        // th2.join();
		// cv::Size image_size = cameras[0].m_ret.size();
		// cv::Size undistorSize = image_size;
		// cv::Mat mapx = cv::Mat(undistorSize,CV_32FC1);
    	// cv::Mat mapy = cv::Mat(undistorSize,CV_32FC1);
		// cv::Mat R = cv::Mat::eye(3,3,CV_32F);
		// cv::Mat optMatrix = getOptimalNewCameraMatrix(intrinsic_matrix, distortion_coeffs, image_size, 1, undistorSize, 0);
		// cv::initUndistortRectifyMap(intrinsic_matrix,distortion_coeffs, R, optMatrix, undistorSize, CV_32FC1, mapx, mapy);
		// cv::Mat t = cv::Mat(undistorSize,CV_8UC3);
        // cv::remap(cameras[0].m_ret,t,mapx, mapy, cv::INTER_CUBIC);
		// t = t(cv::Rect(30,75,1868,930));
		// cv::resize(t, t, cv::Size(1920,1080));
        // imgstither.processnodet(cameras[1].m_ret, cameras[2].m_ret, ret);

		// printf("%p\n", cameras);
		// imgstither.processall(cameras, CAMERA_NUM, ret);
        
        // cv::hconcat(cameras[0].m_ret, cameras[1].m_ret, ret);
        // cv::imshow("m_dev_name", cameras[0].m_ret);
        // ret = cameras[0].m_ret;

        // imgstither.stitcherProcess(cameras, USED_CAMERA_NUM, ret);

        // std::vector<cv::Mat> imgs;
        // for(int i=0;i<USED_CAMERA_NUM;i++)
        //     imgs.push_back(cameras[i].m_ret);

        // // cv::Stitcher cvStitcher = cv::Stitcher::createDefault(false);
        // cv::Stitcher::Status status = stitcher->stitch(imgs, ret);

        // if (status != cv::Stitcher::OK) 
        // {   
        //     std::cout << "Can't stitch images, error code = " << int(status) << std::endl;   
        //     return -1;   
        // }   
        
        if(argc > 1)
        {
            cv::imwrite("final.png", ret);
            return 0;
        }
        else
        {
            cv::imshow("m_dev_name", ret);
            cv::waitKey(33);
        }
    }
    return 0;
}