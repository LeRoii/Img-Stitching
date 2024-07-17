#include "agentfactory.h"
#include "yaml-cpp/yaml.h"
#include "spdlog/spdlog.h"

AgentFactoryBase::AgentFactoryBase()
{

}

AgentFactoryBase::~AgentFactoryBase()
{

}

int AgentFactoryBase::init(const std::string cfgPath)
{
    try
    {
        YAML::Node config = YAML::LoadFile(cfgPath);
        m_usedCamNum = config["USED_CAMERA_NUM"].as<int>();
    }
    catch (YAML::ParserException &ex) {
            spdlog::critical("AgentFactoryBase stitcher cfg yaml:{}  parse failed:{}", cfgPath, ex.what());
            return RET_ERR;
    } catch (YAML::BadFile &ex) {
        spdlog::critical("AgentFactoryBase stitcher cfg yaml:{} load failed:{}", cfgPath, ex.what());
        return RET_ERR;
    }
    // catch(...)
    // {
    //     spdlog::critical("yml parse failed, check your config yaml!");
    //     return RET_ERR;
    // }

    return RET_OK;
}

CameraDispAgentFactory::CameraDispAgentFactory():AgentFactoryBase()
{

}

CameraDispAgentFactory::~CameraDispAgentFactory()
{

}

AgentBase* CameraDispAgentFactory::Create()
{
    // return new CameraDispAgent();
    AgentBase *pRet = nullptr;
    switch (m_usedCamNum)
    {
    case 2:
        pRet = new CameraDispAgent2X();
        break;
    case 4:
        pRet = new CameraDispAgent4X();
        break;
    
    default:
        spdlog::warn("invalid USED_CAMERA_NUM");
        break;
    }

    return pRet;
}

int CameraDispAgentFactory::init(const std::string cfgPath)
{
    return AgentFactoryBase::init(cfgPath);
}