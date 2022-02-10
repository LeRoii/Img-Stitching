#include "panocam.h"
#include "spdlog/spdlog.h"
#include "nvrender.hpp"

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::trace);
    std::string yamlpath = "/home/nvidia/ssd/code/panocam/panocfg.yaml";
    if(argc > 1)
        yamlpath = argv[1];

    panocam *pcam = new panocam(yamlpath);
    cv::Mat frame;
    cv::Mat im = cv::imread("/home/nvidia/ssd/data/11.png");
    // pcam->init(INIT_OFFLINE);
    

    
    spdlog::info("im channel:{}", im.channels());
    cv::cvtColor(im,im,cv::COLOR_RGB2RGBA);
    while(1)
    {
        pcam->getCamFrame(1, frame);
        std::vector<int> ret;
        // pcam->getPanoFrame(frame);

        // pcam->detect(frame, ret);
        spdlog::info("get frame");
        pcam->render(frame);
        // cv::imshow("1", frame);
        // cv::waitKey(1);
    }

    // nvrenderCfg rendercfg{1920, 1080, 960, 540, 0, 0};
    // nvrender *renderer = new nvrender(rendercfg);


    // cv::Mat im = cv::imread("/home/nvidia/ssd/data/11.png");
    // spdlog::info("im channel:{}", im.channels());
    // cv::cvtColor(im,im,cv::COLOR_RGB2RGBA);
    // while(1)
    // {
    //     renderer->render1();
    // }

    return 0;
}