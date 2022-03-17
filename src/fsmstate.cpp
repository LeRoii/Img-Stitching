#include "fsmstate.h"


std::string strPts[3] = {".","..","..."};

namespace panoAPP{

    std::chrono::steady_clock::time_point fsmstate::m_startTimepoint = std::chrono::steady_clock::now();

    fsmstate::fsmstate()
    {
        heartbeat = 0;
    }

    fsmstate::~fsmstate()
    {
        
    }

    void fsmstate::ticktok()
    {
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
        std::chrono::milliseconds time_span = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - m_startTimepoint);
        spdlog::debug("state:{}, ms:{}, diff:{}", m_enStateName, time_span.count(), (time_span-std::chrono::milliseconds{3000}).count());
        if(time_span > std::chrono::milliseconds{1000})
        {
            spdlog::debug("heartbeat");
            m_startTimepoint = std::chrono::steady_clock::now();
            heartbeat++;
        }
    }

    fsmstateStart::fsmstateStart()
    {
        m_enStateName = PANOAPP_STATE_START;
    }

    void fsmstateStart::start()
    {
        cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
        screen.setTo(0);
        double fontScale = 1.2;
        int lineSickness = 2;
        int fontSickness = 2;
        cv::Scalar color = cv::Scalar(5, 217, 82 );
        cv::putText(screen, "initialization...", cv::Point(900, 700), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        pRenderer->showImg(screen);
    }

    panoAPP::enAPPFSMSTATE fsmstateStart::update(panocam *pPanocam)
    {
        ticktok();
        cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
        screen.setTo(0);
        double fontScale = 1.2;
        int lineSickness = 2;
        int fontSickness = 2;
        cv::Scalar color = cv::Scalar(5, 217, 82 );
        std::string dispInitStr = "initialization";
        std::string dispFinalStr;// = "initialization"
                // cv::imshow("a",screen);
        // cv::waitKey(1);

        dispFinalStr = dispInitStr+strPts[heartbeat%3];
        cv::putText(screen, dispFinalStr, cv::Point(900, 700), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
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

    panoAPP::enAPPFSMSTATE fsmstateVerify::update(panocam *pPanocam)
    {
        spdlog::debug("state [{}] update", m_enStateName);
        ticktok();
        cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
        screen.setTo(0);
        double fontScale = 1.2;
        int lineSickness = 2;
        int fontSickness = 2;
        cv::Scalar color = cv::Scalar(5, 217, 82 );
        std::string dispInitStr = "verification";
        std::string dispFinalStr;// = "initialization"
        dispFinalStr = dispInitStr+strPts[heartbeat%3];
        cv::putText(screen, dispFinalStr, cv::Point(900, 700), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        pRenderer->showImg(screen);

        if(heartbeat > 3)
        {
            if(pPanocam->verify())
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

    panoAPP::enAPPFSMSTATE fsmstateInit::update(panocam *pPanocam)
    {
        spdlog::debug("state [{}] update", m_enStateName);
    }

    void fsmstateInit::stop()
    {

    }

    void fsmstateRun::start()
    {

    }

    panoAPP::enAPPFSMSTATE fsmstateRun::update(panocam *pPanocam)
    {

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

    panoAPP::enAPPFSMSTATE fsmstateFinish::update(panocam *pPanocam)
    {
        spdlog::debug("state [{}] update", m_enStateName);
        ticktok();
        cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
        screen.setTo(0);
        double fontScale = 1.2;
        int lineSickness = 2;
        int fontSickness = 2;
        cv::Scalar color = cv::Scalar(5, 217, 82 );
        std::string dispInitStr = "verification";
        std::string dispFinalStr;// = "initialization"
        dispFinalStr = dispInitStr+strPts[heartbeat%3];
        cv::putText(screen, "verification failed!", cv::Point(850, 700), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        pRenderer->showImg(screen);
    }

    void fsmstateFinish::stop()
    {

    }
    
}