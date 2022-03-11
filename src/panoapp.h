#ifndef _PANPAPP_H_
#define _PANPAPP_H_
#include "context.h"

namespace panoAPP{
    class Factory
        {
        public :
            static fsmstate* CreateState(panoAPP::context* context, panoAPP::enAPPFSMSTATE name)
            {
                fsmstate* state = nullptr;
                if (name == panoAPP::PANOAPP_STATE_START)
                {
                    state = new panoAPP::fsmstateStart();
                }
                else if (name == panoAPP::PANOAPP_STATE_VERIFY)
                {
                    state = new panoAPP::fsmstateVerify();
                }
                else if (name == panoAPP::PANOAPP_STATE_INIT)
                {
                    state = new panoAPP::fsmstateInit();
                }
                else if (name == panoAPP::PANOAPP_STATE_RUN)
                {
                    state = new panoAPP::fsmstateRun();
                }
        
                context->RegisterState(name, state);
                return state;
            }
        };
}


#endif