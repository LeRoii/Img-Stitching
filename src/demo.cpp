#include "panocam.h"
#include "spdlog/spdlog.h"

int main()
{
    panocam *pcam = new panocam();
    pcam->init(INIT_OFFLINE);
    cv::Mat frame;
    while(1)
    {
        // pcam->getCamFrame(1, frame);
        pcam->getPanoFrame(frame);
        spdlog::info("get frame");
        cv::imshow("1", frame);
        cv::waitKey(1);
    }
    return 0;
}