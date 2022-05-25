#ifndef _CONTEXT_H_
#define _CONTEXT_H_


#include <unordered_map>
#include "fsmstate.h"

namespace panoAPP{
    class fsmstate;
    class context
    {
        public:
            context(std::string yamlpath);
            ~context();
            void update();
            void RegisterState(panoAPP::enAPPFSMSTATE statename, fsmstate* state);
            void start(panoAPP::enAPPFSMSTATE statename);

        private:
            void ticktok();
            std::unordered_map<panoAPP::enAPPFSMSTATE, fsmstate*> m_states;
            panoAPP::enAPPFSMSTATE m_enCurStateName;
            fsmstate *m_pCurState;
            panocamimpl *m_pPanocam;
            static std::chrono::steady_clock::time_point m_startTimepoint;
            int m_heartbeat;
            

    };
}

#endif