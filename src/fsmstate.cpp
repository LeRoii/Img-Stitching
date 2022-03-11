#include "fsmstate.h"

namespace panoAPP{

    fsmstate::fsmstate()
    {
        
    }

    fsmstate::~fsmstate()
    {

    }

    void fsmstateStart::start()
    {
        cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
        screen.setTo(0);
        double fontScale = 0.6;
        int lineSickness = 2;
        int fontSickness = 2;
        cv::Scalar color = cv::Scalar(5, 217, 82 );
        cv::putText(screen, "initialization...", cv::Point(900, 700), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        pRenderer->showImg(screen);
    }

    panoAPP::enAPPFSMSTATE fsmstateStart::update()
    {
        cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
        screen.setTo(0);
        double fontScale = 0.6;
        int lineSickness = 2;
        int fontSickness = 2;
        cv::Scalar color = cv::Scalar(5, 217, 82 );
        cv::putText(screen, "initialization...", cv::Point(900, 700), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        pRenderer->showImg(screen);
        // cv::imshow("a",screen);
        // cv::waitKey(1);
    }

    void fsmstateStart::stop()
    {

    }

    void fsmstateVerify::start()
    {

    }

    panoAPP::enAPPFSMSTATE fsmstateVerify::update()
    {

    }

    void fsmstateVerify::stop()
    {

    }

    void fsmstateInit::start()
    {

    }

    panoAPP::enAPPFSMSTATE fsmstateInit::update()
    {

    }

    void fsmstateInit::stop()
    {

    }

    void fsmstateRun::start()
    {

    }

    panoAPP::enAPPFSMSTATE fsmstateRun::update()
    {

    }

    void fsmstateRun::stop()
    {

    }


    
}