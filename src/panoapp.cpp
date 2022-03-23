#include "yaml-cpp/yaml.h"
#include "panocam.h"
#include "panoapp.h"



nvrender *pRenderer;

int main(int argc, char *argv[])
{
    spdlog::set_level(spdlog::level::debug);
    std::string yamlpath = "/home/nvidia/ssd/code/0209is/cfg/pamocfg.yaml";
    if(argc > 1)
        yamlpath = argv[1];
    YAML::Node config = YAML::LoadFile(yamlpath);
    

    renderWidth = config["renderWidth"].as<int>();
    renderHeight = config["renderHeight"].as<int>();
    nvrenderCfg rendercfg{renderBufWidth, renderBufHeight, renderWidth, renderHeight, renderX, renderY, renderMode};
    pRenderer = new nvrender(rendercfg);

    panoAPP::context *appctx = new panoAPP::context();
    panoAPP::Factory::CreateState(appctx, panoAPP::PANOAPP_STATE_START);
    panoAPP::Factory::CreateState(appctx, panoAPP::PANOAPP_STATE_VERIFY);
    panoAPP::Factory::CreateState(appctx, panoAPP::PANOAPP_STATE_INIT);
    panoAPP::Factory::CreateState(appctx, panoAPP::PANOAPP_STATE_RUN);
    panoAPP::Factory::CreateState(appctx, panoAPP::PANOAPP_STATE_FINISH);

    appctx->start(panoAPP::PANOAPP_STATE_START);

    while(1)
    {
        appctx->update();
    }

    cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
    screen.setTo(0);

    double fontScale = 0.6;
    int lineSickness = 2;
    int fontSickness = 2;
    cv::Scalar color = cv::Scalar(5, 217, 82 );
    cv::putText(screen, "initialization...", cv::Point(900, 700), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
    while(1)
        pRenderer->showImg(screen);

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
        pRenderer->render(frame);
        // pcam->render(frame);
        // cv::imshow("1", frame);
        // cv::waitKey(1);
    }
    return 0;
}