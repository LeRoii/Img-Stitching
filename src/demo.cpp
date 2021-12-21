#include "panocam.h"
#include "spdlog/spdlog.h"

int main(int argc, char *argv[])
{
    std::string yamlpath = "/home/nvidia/ssd/code/0929IS/cfg/pamocfg.yaml";
    if(argc > 1)
        yamlpath = argv[1];

    panocam *pcam = new panocam(yamlpath);
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