#include "fsmstate.h"

extern int g_usrcmd;

std::string strPts[3] = {".","..","..."};

namespace panoAPP{

    

    fsmstate::fsmstate()
    {
    }

    fsmstate::~fsmstate()
    {
        
    }

    fsmstateStart::fsmstateStart()
    {
        m_enStateName = PANOAPP_STATE_START;
    }

    void fsmstateStart::start()
    {
        // cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
        // screen.setTo(0);
        // double fontScale = 1.2;
        // int lineSickness = 2;
        // int fontSickness = 2;
        // cv::Scalar color = cv::Scalar(5, 217, 82 );
        // cv::putText(screen, "initialization...", cv::Point(900, 700), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        // pRenderer->showImg(screen);
    }

    panoAPP::enAPPFSMSTATE fsmstateStart::update(panocamimpl *pPanocam, int heartbeat)
    {
        spdlog::debug("state [{}] update, heartbeat:[{}]", m_enStateName, heartbeat);
        cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
        screen.setTo(0);
        double fontScale = 1.2;
        int lineSickness = 2;
        int fontSickness = 2;
        cv::Scalar color = cv::Scalar(5, 217, 82 );
        std::string dispInitStr = "xjtu panorama app start";
        std::string dispFinalStr;// = "initialization"
                // cv::imshow("a",screen);
        // cv::waitKey(1);
        cv::Size testsz = cv::getTextSize(dispInitStr, cv::FONT_HERSHEY_SIMPLEX, fontScale, fontSickness, 0);
        cv::Point textpos((renderBufWidth - testsz.width)/2, (renderBufHeight - testsz.height)/2+200);

        dispFinalStr = dispInitStr+strPts[heartbeat%3];
        cv::putText(screen, dispFinalStr, textpos, cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        pRenderer->showImg(screen);

        if(heartbeat > 3)
        {
            spdlog::debug("fsmstateStart update time out");
            return PANOAPP_STATE_VERIFY;
        }
        else
            return PANOAPP_STATE_START;
    }

    void fsmstateStart::stop()
    {
        spdlog::debug("state [{}] stop", m_enStateName);
    }

    fsmstateVerify::fsmstateVerify()
    {
        m_enStateName = PANOAPP_STATE_VERIFY;
    }

    void fsmstateVerify::start()
    {
        spdlog::debug("state [{}] start", m_enStateName);
    }

    panoAPP::enAPPFSMSTATE fsmstateVerify::update(panocamimpl *pPanocam, int heartbeat)
    {
        spdlog::debug("state [{}] update, heartbeat:[{}]", m_enStateName, heartbeat);
        static int initHeartbeat = heartbeat;
        cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
        screen.setTo(0);
        double fontScale = 1.2;
        int lineSickness = 2;
        int fontSickness = 2;
        cv::Scalar color = cv::Scalar(5, 217, 82 );
        std::string dispInitStr = "verification";
        std::string dispFinalStr;// = "initialization"
        dispFinalStr = dispInitStr+strPts[heartbeat%3];
        cv::Size testsz = cv::getTextSize(dispInitStr, cv::FONT_HERSHEY_SIMPLEX, fontScale, fontSickness, 0);
        cv::Point textpos((renderBufWidth - testsz.width)/2, (renderBufHeight - testsz.height)/2+200);
        cv::putText(screen, dispFinalStr, textpos, cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        pRenderer->showImg(screen);

        if(heartbeat > initHeartbeat+3)
        {
            if(!pPanocam->verify())
            {
                spdlog::debug("verify failed");
                return PANOAPP_STATE_FINISH;
            }
            else
            {
                spdlog::debug("verify succeed");
                return PANOAPP_STATE_INIT;
            }

            return PANOAPP_STATE_VERIFY;
        }
    }

    void fsmstateVerify::stop()
    {
        spdlog::debug("state [{}] stop", m_enStateName);
    }

    fsmstateInit::fsmstateInit()
    {
        m_enStateName = PANOAPP_STATE_INIT;
    }

    void fsmstateInit::start()
    {

    }

    panoAPP::enAPPFSMSTATE fsmstateInit::update(panocamimpl *pPanocam, int heartbeat)
    {
        spdlog::debug("state [{}] update, heartbeat:[{}]", m_enStateName, heartbeat);
        cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
        screen.setTo(0);
        double fontScale = 1.2;
        int lineSickness = 2;
        int fontSickness = 2;
        cv::Scalar color = cv::Scalar(5, 217, 82 );
        std::string dispInitStr = "initalization...";
        std::string dispFinalStr;// = "initialization"
        dispFinalStr = dispInitStr+strPts[heartbeat%3];
        cv::Size testsz = cv::getTextSize(dispInitStr, cv::FONT_HERSHEY_SIMPLEX, fontScale, fontSickness, 0);
        cv::Point textpos((renderBufWidth - testsz.width)/2, (renderBufHeight - testsz.height)/2+200);
        cv::putText(screen, "initalization...", textpos, cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        pRenderer->showImg(screen);

        int ret = pPanocam->init();
        if(ret == RET_OK)
        {
            spdlog::debug("init succeed");
            return PANOAPP_STATE_RUN;
        }
        else
        {
            spdlog::debug("init failed");
            return PANOAPP_STATE_FINISH;
        }
    }

    void fsmstateInit::stop()
    {

    }

    fsmstateRun::fsmstateRun()
    {
        m_enStateName = PANOAPP_STATE_RUN;
    }

    void fsmstateRun::start()
    {
        pRenderer->drawIndicator();
        int ret = m_usbctrler.init();
        if(ret == RET_OK)
            spdlog::debug("usb ctrler init ok");
        else
            spdlog::debug("usb ctrler init failed");
        spdlog::info("app started");
    }

    panoAPP::enAPPFSMSTATE fsmstateRun::update(panocamimpl *pPanocam, int heartbeat)
    {
        spdlog::debug("state [{}] update, heartbeat:[{}]", m_enStateName, heartbeat);
        cv::Mat frame;
        uint8_t displaymode = pPanocam->sysStatus().displayMode;
        static uint8_t lastDisplayMode = displaymode;
        switch(displaymode)
        {
            case 0xC1:
            case 0xC2:
            case 0xC3:
            case 0xC4:
            case 0xC5:
            case 0xC6:
            case 0xC7:
            case 0xC8:pPanocam->getCamFrame(displaymode & 0xf, frame);break;
            case 0xCB:m_usbctrler.trigger();displaymode = pPanocam->sysStatus().displayMode = 0xCA;
            case 0xCA:
            default:pPanocam->getPanoFrame(frame);break;
        }

        if(getbit(g_usrcmd, SETTING_ON))
        {
            if(getbit(g_usrcmd, SETTING_IMGENHANCE))
                pPanocam->sysStatus().enhancementTrigger = !pPanocam->sysStatus().enhancementTrigger;
            if(getbit(g_usrcmd, SETTING_DETECTION))
                pPanocam->sysStatus().detectionTrigger = !pPanocam->sysStatus().detectionTrigger; 
            if(getbit(g_usrcmd, SETTING_CROSS))
                pPanocam->sysStatus().crossTrigger = !pPanocam->sysStatus().crossTrigger;
            if(getbit(g_usrcmd, SETTING_SAVE))
                pPanocam->sysStatus().saveTrigger = !pPanocam->sysStatus().saveTrigger;
            g_usrcmd = 0;
        }

        if(pPanocam->sysStatus().enhancementTrigger)
            pPanocam->imgEnhancement(frame);
        if(pPanocam->sysStatus().detectionTrigger)
            pPanocam->detect(frame);
        if(pPanocam->sysStatus().crossTrigger)
            pPanocam->drawCross(frame);
        // if(pPanocam->sysStatus().saveTrigger)
        if(1)
        {
            // cv::resize(frame, frame, cv::Size(1280, 720));
            pPanocam->saveAndSend(frame);
            // pPanocam->sysStatus().saveTrigger = !pPanocam->sysStatus().saveTrigger;
        }
        if(lastDisplayMode != displaymode && displaymode == 0xCA)
            pRenderer->drawIndicator();
        lastDisplayMode = displaymode;

        if(pPanocam->sysStatus().zoomTrigger)
        {
            cv::Mat innerFrame;
            int x = pPanocam->sysStatus().zoomPointX;
            int y = pPanocam->sysStatus().zoomPointY;
            int maxX = frame.cols - 300;
            int minX = 150;
            int maxY = frame.rows - 300;
            int minY = 150;
            int w = frame.cols;
            int h = frame.rows;
            int camIdx = 0;
           

            if(y<h/2)
            {
                if(x < w/4)
                    camIdx =  1;
                else if(x < w/2)
                    camIdx =  2;
                else if(x < w *3/4)
                    camIdx =  3;
                else
                    camIdx =  4;
            }
            else
            {
                if(x < w/4)
                    camIdx =  5;
                else if(x < w/2)
                    camIdx =  6;
                else if(x < w *3/4)
                    camIdx =  7;
                else
                    camIdx =  8;
            }

            int cropX, cropY;
            if(x%(w/4) < (w/4))
                cropX = 0;
            else
                cropX = 960;

            if(y%(h/4) < (h/4))
                cropY = 0;
            else
                cropY = 540;

            // x = (x >= minX ? x : minX);
            x = (x <= maxX ? x : maxX);
            // y = (y >= minY ? x : minY);
            y = (y <= maxY ? y : maxY);

            pPanocam->getCamFrame(camIdx, innerFrame);
            innerFrame = innerFrame(cv::Rect(cropX,cropY,960,540));
            cv::resize(innerFrame, innerFrame, cv::Size(300,300));
            pRenderer->renderimgs(frame, innerFrame, x, y);
        }
        else
            pRenderer->render(frame);

        return PANOAPP_STATE_RUN;
    }

    void fsmstateRun::stop()
    {

    }

    fsmstateFinish::fsmstateFinish()
    {
        m_enStateName = PANOAPP_STATE_FINISH;
    }

    void fsmstateFinish::start()
    {
        spdlog::debug("state [{}] start", m_enStateName);
    }

    panoAPP::enAPPFSMSTATE fsmstateFinish::update(panocamimpl *pPanocam, int heartbeat)
    {
        spdlog::debug("state [{}] update, heartbeat:[{}]", m_enStateName, heartbeat);
        cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
        screen.setTo(0);
        double fontScale = 1.2;
        int lineSickness = 2;
        int fontSickness = 2;
        cv::Scalar color = cv::Scalar(5, 217, 82 );
        std::string dispFinalStr;// = "initialization"

        uint8_t status = pPanocam->getStatus();
        if(status == STATUS_VERIFICATION_FAILED)
            dispFinalStr = "verification failed!";
        else if(status == STATUS_INITALIZATION_FAILED)
            dispFinalStr = "initialization failed!";

        cv::putText(screen, dispFinalStr, cv::Point(850, 700), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        pRenderer->showImg(screen);
    }

    void fsmstateFinish::stop()
    {

    }
    
}