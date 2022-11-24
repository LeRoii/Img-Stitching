#ifndef _AGENT_H_
#define _AGENT_H_

#include "stitcherglobal.h"

#include "nvcam.hpp"
#include "imageProcess.h"
#include "nvrenderAlpha.h"
#include <vector>

//abstract agent class 
class AgentBase
{
public:
    AgentBase();
    virtual ~AgentBase();
    virtual int init(const std::string cfgPath);
    virtual int run() = 0;

protected:
    int parseYml(const std::string cfgPath);
    // stCamCfg m_stCameraCfg;
    stNvrenderCfg m_stRenderCfg;
    stAgentCfg m_stAgentCfg;
    std::vector<std::shared_ptr<nvCam>> m_apCameras;
    std::vector<std::thread> m_aCamThreads;

    nvrenderAlpha *m_pRender;
    imageProcessor *m_pnvProcessor;
};

class PanoAgent : public AgentBase
{
public:
    PanoAgent();
    ~PanoAgent();

};

//abstract camera display agent class
class CameraDispAgent : public AgentBase
{
public:
    CameraDispAgent();
    virtual ~CameraDispAgent();

    virtual int init(const std::string cfgPath);
};

//camera display agent for 2 cameras
class CameraDispAgent2X : public CameraDispAgent
{
public:
    CameraDispAgent2X();
    ~CameraDispAgent2X();

    int init(const std::string cfgPath);
    int run();
};

//camera display agent for 4 cameras
class CameraDispAgent4X : public CameraDispAgent
{
public:
    CameraDispAgent4X();
    ~CameraDispAgent4X();

    int init(const std::string cfgPath);
    int run();
};

#endif