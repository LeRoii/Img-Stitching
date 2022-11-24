#ifndef _AGENT_FACTORY_H_
#define _AGENT_FACTORY_H_

#include "agent.h"
#include "stitcherglobal.h"
#include <vector>

//abstract facroty class
class AgentFactoryBase
{
public:
    AgentFactoryBase();
    virtual ~AgentFactoryBase();
    virtual AgentBase* Create() = 0;
    virtual int init(const std::string cfgPath);

protected:
    int m_usedCamNum;
};

class PanoAgentFactory : public AgentFactoryBase
{
public:
    PanoAgentFactory();
    ~PanoAgentFactory();
};

//use this class to produce all kinds of camera display agent
class CameraDispAgentFactory : public AgentFactoryBase
{
public:
    CameraDispAgentFactory();
    ~CameraDispAgentFactory();
    AgentBase* Create();
    int init(const std::string cfgPath);
};



#endif