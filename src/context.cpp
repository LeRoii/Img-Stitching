#include "context.h"

namespace panoAPP{
    std::chrono::steady_clock::time_point context::m_startTimepoint = std::chrono::steady_clock::now();
    context::context(std::string yamlpath):m_pCurState(nullptr),m_heartbeat(0)
    {
        // m_startTimapoint = std::chrono::steady_clock::now();
        m_pPanocam = new panocamimpl(yamlpath);
    }

    context::~context()
    {
        for (auto iter : m_states)
		{
			if (iter.second != nullptr)
			{
				delete iter.second;
				iter.second = nullptr;
			}
		}
		m_states.clear();
    }

    void context::RegisterState(panoAPP::enAPPFSMSTATE statename, fsmstate* state)
    {
        m_states.emplace(statename, state);
    }

    void context::start(panoAPP::enAPPFSMSTATE statename)
    {
        std::unordered_map<panoAPP::enAPPFSMSTATE, fsmstate*>::iterator iter_map = m_states.find(statename);
		if (iter_map != m_states.end())
		{
			m_enCurStateName = iter_map->first;
            m_pCurState = iter_map->second;
			iter_map->second->start();
		}
    }

    void context::ticktok()
    {
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
        std::chrono::milliseconds time_span = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - m_startTimepoint);
        spdlog::trace("ms:{}, diff:{}", time_span.count(), (time_span-std::chrono::milliseconds{3000}).count());
        if(time_span > std::chrono::milliseconds{1000})
        {
            spdlog::debug("heartbeat");
            m_startTimepoint = std::chrono::steady_clock::now();
            m_heartbeat++;
        }
    }

    void context::update()
    {
        static int lastheartbeat = 0;
        ticktok();
        auto next = m_enCurStateName;
        if(m_pCurState != nullptr)
            next = m_pCurState->update(m_pPanocam, m_heartbeat);

        if(next != m_enCurStateName)
        {
            spdlog::debug("state transition");
            m_pCurState->stop();
            start(next);
            m_enCurStateName = next;
        }
        if(lastheartbeat != m_heartbeat)
        {
            m_pPanocam->sendStatus();
            lastheartbeat = m_heartbeat;
        }



        // std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
        // std::chrono::milliseconds time_span = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - m_startTimapoint);
        // // std::cout << "ms: " << time_span.count() << std::endl;
        // spdlog::info("state:{}, ms:{}", m_enCurStateName, time_span.count());
        // if(time_span.count() > 3000)
    }



} // namespace panoAPP
