#ifndef _FSMSTATE_H_
#define _FSMSTATE_H_
#include "panocamimpl.h"
#include "nvrender.h"
#include "spdlog/spdlog.h"
#include <chrono>


#define reversebit(x,y)  x^=(1<<y)
#define getbit(x,y)   ((x) >> (y)&1)
#define setbit(x,y) x|=(1<<y)
#define clrbit(x,y) x&=~(1<<y)

const int SETTING_IMGENHANCE = 0;
const int SETTING_DETECTION = 1;
const int SETTING_CROSS = 2;
const int SETTING_SAVE = 3;
const int SETTING_ON = 31;

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
            virtual panoAPP::enAPPFSMSTATE update(panocamimpl *pPanocam, int heartbeat)=0;
            virtual void stop()=0;

        protected:
           panoAPP::enAPPFSMSTATE m_enStateName; 
        //    static std::chrono::steady_clock::time_point m_startTimepoint;
        //    int heartbeat;
    };

    class fsmstateStart : public fsmstate
    {
        public:
        fsmstateStart();
        void start();
        panoAPP::enAPPFSMSTATE update(panocamimpl *pPanocam, int heartbeat);
        void stop();
    };

    class fsmstateVerify : public fsmstate
    {
        public:
        fsmstateVerify();
        void start();
        panoAPP::enAPPFSMSTATE update(panocamimpl *pPanocam, int heartbeat);
        void stop();
    };

    class fsmstateInit : public fsmstate
    {
        public:
        fsmstateInit();
        void start();
        panoAPP::enAPPFSMSTATE update(panocamimpl *pPanocam, int heartbeat);
        void stop();
    };

    class fsmstateRun : public fsmstate
    {
        void start();
        panoAPP::enAPPFSMSTATE update(panocamimpl *pPanocam, int heartbeat);
        void stop();
    };

    class fsmstateFinish : public fsmstate
    {
        public:
        fsmstateFinish();
        void start();
        panoAPP::enAPPFSMSTATE update(panocamimpl *pPanocam, int heartbeat);
        void stop();
    };
}

#endif