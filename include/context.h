#ifndef _CONTEXT_H_
#define _CONTEXT_H_


#include <unordered_map>


#include "fsmstate.h"

namespace panoAPP{
    class fsmstate;
    class context
    {
        public:
            context();
            ~context();
            void update();
            void RegisterState(panoAPP::enAPPFSMSTATE statename, fsmstate* state);
            void start(panoAPP::enAPPFSMSTATE statename);

        private:
            std::unordered_map<panoAPP::enAPPFSMSTATE, fsmstate*> m_states;
            panoAPP::enAPPFSMSTATE m_enCurStateName;
            fsmstate *m_pCurState;
            panocam *m_pPanocam;
            

    };
}

#endif