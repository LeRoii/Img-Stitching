#include "panocam.h"
#include "spdlog/spdlog.h"

int main()
{
    std::string net ="/home/nvidia/ssd/code/cameracap/cfg/yolo4_berkeley_fp16.rt" ; //yolo4_320_fp16.rt（44ms, double detect）, yolo4_berkeley_fp16.rt(64ms),  kitti_yolo4_int8.rt 
    std::string cfgpath = "/home/nvidia/ssd/code/0929IS/cfg/";


    panocam *pcam = new panocam(3840, 2160, net, cfgpath);
    pcam->init(INIT_OFFLINE);
    cv::Mat frame;
    while(1)
    {
        // pcam->getCamFrame(1, frame);
        std::vector<int> ret;
        pcam->getPanoFrame(frame);
        pcam->detect(frame, ret);
        spdlog::info("get frame");
        cv::imshow("1", frame);
        cv::waitKey(1);
    }
    return 0;
}