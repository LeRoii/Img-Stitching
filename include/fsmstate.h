#ifndef _FSMSTATE_H_
#define _FSMSTATE_H_
#include "nvrender.h"
#include "spdlog/spdlog.h"

extern nvrender *pRenderer;

namespace panoAPP{

    enum enAPPFSMSTATE
    {
        PANOAPP_STATE_START = 1,
        PANOAPP_STATE_VERIFY = 2,
        PANOAPP_STATE_INIT = 3,
        PANOAPP_STATE_RUN = 4
    };

    class fsmstate
    {
        public:
            fsmstate();
            ~fsmstate();

            virtual void start()=0;
            virtual panoAPP::enAPPFSMSTATE update()=0;
            virtual void stop()=0;

        private:
           panoAPP::enAPPFSMSTATE m_enStateName; 
    };

    class fsmstateStart : public fsmstate
    {
        void start();
        panoAPP::enAPPFSMSTATE update();
        void stop();
    };

    class fsmstateVerify : public fsmstate
    {
        void start();
        panoAPP::enAPPFSMSTATE update();
        void stop();
    };

    class fsmstateInit : public fsmstate
    {
        void start();
        panoAPP::enAPPFSMSTATE update();
        void stop();
    };

    class fsmstateRun : public fsmstate
    {
        void start();
        panoAPP::enAPPFSMSTATE update();
        void stop();
    };

    
}

#endif