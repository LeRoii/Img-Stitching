#include "panocam.h"
#include "spdlog/spdlog.h"
#include "nvrender.hpp"

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::info);
    std::string yamlpath = "/home/nvidia/ssd/code/0209is/cfg/pamocfg.yaml";
    if(argc > 1)
        yamlpath = argv[1];

    panocam *pcam = new panocam(yamlpath);
    cv::Mat frame;
    pcam->init(INIT_OFFLINE);
    while(1)
    {
        // pcam->getCamFrame(1, frame);
        std::vector<int> ret;
        pcam->getPanoFrame(frame);

        // pcam->detect(frame, ret);
        spdlog::info("get frame");
        pcam->render(frame);
        // cv::imshow("1", frame);
        // cv::waitKey(1);
    }
    return 0;
}