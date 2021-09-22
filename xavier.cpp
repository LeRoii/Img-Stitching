#include <opencv2/opencv.hpp>
#include <thread>
#include "stitcher.hpp"
#include <string>
#include "ocvstitcher.hpp"

#define USED_CAMERA_NUM 2

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

    // stitcherCfg stitchercfg;
    // stitchercfg.imgHeight = camcfgs[0].retHeight;
    // stitchercfg.imgWidth = camcfgs[0].retWidth;

    // stitcher imgstither(stitchercfg, 1);

    // cv::Ptr<cv::Stitcher> stitcher = cv::Stitcher::create(cv::Stitcher::PANORAMA, true);

    ocvStitcher ostitcher;

    vector<Mat> imgs;
    imgs.push_back(imread("./calibpana/1-dist.png"));
    imgs.push_back(imread("./calibpana/2-dist.png"));

    ostitcher.init(imgs);
    ostitcher.process(imgs, ret);

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

        Mat rett;
        vector<Mat> imgss(2);
        imgss[0] = cameras[0].m_ret.clone();
        imgss[1] = cameras[1].m_ret.clone();
        ostitcher.process(imgss, rett);
            

        // std::thread th1(&gmslCamera::read_frame, std::ref(cameras[0]));
        // th1.join();
        // std::thread th2(&gmslCamera::read_frame, std::ref(cameras[1]));
        // th2.join();
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
            cv::imwrite("final.png", rett);
            return 0;
        }
        else
        {
            // cv::imshow("m_dev_name", rett);
            // cv::waitKey(330);
        }
    }
    return 0;
}