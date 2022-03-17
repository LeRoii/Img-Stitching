#ifndef _FSMSTATE_H_
#define _FSMSTATE_H_
#include "nvrender.h"
#include "spdlog/spdlog.h"
#include <chrono>
#include "panocam.h"

extern nvrender *pRenderer;

namespace panoAPP{

    enum enAPPFSMSTATE
    {
        PANOAPP_STATE_START = 1,
        PANOAPP_STATE_VERIFY = 2,
        PANOAPP_STATE_INIT = 3,
        PANOAPP_STATE_RUN = 4,
        PANOAPP_STATE_FINISH = 5
    };

    class fsmstate
    {
        public:
            fsmstate();
            ~fsmstate();

            virtual void start()=0;
            virtual panoAPP::enAPPFSMSTATE update(panocam *pPanocam)=0;
            virtual void stop()=0;
            void ticktok();

        protected:
           panoAPP::enAPPFSMSTATE m_enStateName; 
           static std::chrono::steady_clock::time_point m_startTimepoint;
           int heartbeat;
    };

    class fsmstateStart : public fsmstate
    {
        public:
        fsmstateStart();
        void start();
        panoAPP::enAPPFSMSTATE update(panocam *pPanocam);
        void stop();
    };

    class fsmstateVerify : public fsmstate
    {
        public:
        fsmstateVerify();
        void start();
        panoAPP::enAPPFSMSTATE update(panocam *pPanocam);
        void stop();
    };

    class fsmstateInit : public fsmstate
    {
        public:
        fsmstateInit();
        void start();
        panoAPP::enAPPFSMSTATE update(panocam *pPanocam);
        void stop();
    };

    class fsmstateRun : public fsmstate
    {
        void start();
        panoAPP::enAPPFSMSTATE update(panocam *pPanocam);
        void stop();
    };

    class fsmstateFinish : public fsmstate
    {
        public:
        fsmstateFinish();
        void start();
        panoAPP::enAPPFSMSTATE update(panocam *pPanocam);
        void stop();
    };

    
}

#endif