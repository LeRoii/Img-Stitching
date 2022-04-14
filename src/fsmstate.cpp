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

    void fsmstateRun::start()
    {
        pRenderer->drawIndicator();
        m_enStateName = PANOAPP_STATE_RUN;
        spdlog::info("app started");
    }

    panoAPP::enAPPFSMSTATE fsmstateRun::update(panocamimpl *pPanocam, int heartbeat)
    {
        spdlog::debug("state [{}] update, heartbeat:[{}]", m_enStateName, heartbeat);
        cv::Mat frame;
        pPanocam->getPanoFrame(frame);

        // getbit(g_usrcmd, SETTING_IMGENHANCE) ? m_stSysStatus.imgEnhance = true : m_stSysStatus.imgEnhance = false;
        // getbit(g_usrcmd, SETTING_DETECTION) ? m_stSysStatus.detection = true : m_stSysStatus.detection = false;
        // getbit(g_usrcmd, SETTING_CROSS) ? m_stSysStatus.cross = true : m_stSysStatus.cross = false;

        if(getbit(g_usrcmd, SETTING_IMGENHANCE) != m_stSysStatus.imgEnhance)
        {
            spdlog::info("image enhancement switch to [{}]", !m_stSysStatus.imgEnhance);
            m_stSysStatus.imgEnhance = getbit(g_usrcmd, SETTING_IMGENHANCE);
        }
        if(getbit(g_usrcmd, SETTING_DETECTION) != m_stSysStatus.detection)
        {
            spdlog::info("image detection switch to [{}]", !m_stSysStatus.detection);
            m_stSysStatus.detection = getbit(g_usrcmd, SETTING_DETECTION);
        }
        if(getbit(g_usrcmd, SETTING_CROSS) != m_stSysStatus.cross)
        {
            spdlog::info("image cross switch to [{}]", !m_stSysStatus.cross);
            m_stSysStatus.cross = getbit(g_usrcmd, SETTING_CROSS);
        }
        if(getbit(g_usrcmd, SETTING_SAVE) != m_stSysStatus.save)
        {
            spdlog::info("image save switch to [{}]", !m_stSysStatus.save);
            m_stSysStatus.save = getbit(g_usrcmd, SETTING_SAVE);
        }

        if(m_stSysStatus.imgEnhance)
            pPanocam->imgEnhancement(frame);
        if(m_stSysStatus.detection)
            pPanocam->detect(frame);
        if(m_stSysStatus.cross)
            pPanocam->drawCross(frame);
        if(m_stSysStatus.save)
            pPanocam->saveAndSend(frame);
        
        // if(getbit(g_usrcmd, SETTING_IMGENHANCE))
        // {
        //     pPanocam->imgEnhancement(frame);
        //     m_stSysStatus.imgEnhance = true;
        // }
        // if(getbit(g_usrcmd, SETTING_DETECTION))
        // {
        //     pPanocam->detect(frame);
        //     m_stSysStatus.detection = true;
        // }
        // if(getbit(g_usrcmd, SETTING_CROSS))
        //     pPanocam->drawCross(frame);
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