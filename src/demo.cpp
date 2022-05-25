#include "panocam.h"
#include "spdlog/spdlog.h"

int main(int argc, char *argv[])
{
    std::string yamlpath = "../cfg/pamocfg.yaml";
    if(argc > 1)
        yamlpath = argv[1];

    panocam *pcam = new panocam(yamlpath);
    cv::Mat frame;
    if(-1 == pcam->init())
    {
        spdlog::critical("init failed, exit");
        return 0;
    }
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