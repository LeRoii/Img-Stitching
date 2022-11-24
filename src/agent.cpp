#include "agent.h"

AgentBase::AgentBase()
{

}

AgentBase::~AgentBase()
{

}

int AgentBase::init(const std::string stitcherCfgPath)
{
    if(RET_ERR == parseYml(stitcherCfgPath))
        return RET_ERR;
    
    m_pRender = new nvrenderAlpha(m_stRenderCfg);

    for(int i=0;i<m_stAgentCfg.usedCamNum;i++)
    {
        // camcfgs[i] = m_stCameraCfg;
        // camcfgs[i].id = i;
        // m_stCameraCfg.id = i;
        // std::string str = "/dev/video";
        // strcpy(m_stCameraCfg.name, (str+std::to_string(i)).c_str());
        m_apCameras.push_back(std::make_shared<nvCam>(i));
        if(RET_ERR == m_apCameras[i]->init(stitcherCfgPath))
        {
            spdlog::warn("camera [{}] init failed!", i);
        }
        m_aCamThreads.push_back(std::thread(&nvCam::run, m_apCameras[i].get()));
        m_aCamThreads[i].detach();
    }

    m_pnvProcessor = new imageProcessor();
    m_pnvProcessor->init(stitcherCfgPath);

    //encoder?
}

int AgentBase::parseYml(const std::string stitcherCfgPath)
{
    try
    {
        YAML::Node config = YAML::LoadFile(stitcherCfgPath);

        num_images = config["num_images"].as<int>();

        m_stRenderCfg.renderw = config["renderWidth"].as<int>();
        m_stRenderCfg.renderh = config["renderHeight"].as<int>();
        m_stRenderCfg.renderx = config["renderX"].as<int>();
        m_stRenderCfg.rendery = config["renderY"].as<int>();
        m_stRenderCfg.bufferw = config["renderBufWidth"].as<int>();
        m_stRenderCfg.bufferh = config["renderBufHeight"].as<int>();
        m_stRenderCfg.mode = config["renderMode"].as<int>();

        std::string loglvl = config["loglvl"].as<std::string>();
        if(loglvl == "critical")
            spdlog::set_level(spdlog::level::critical);
        else if(loglvl == "trace")
            spdlog::set_level(spdlog::level::trace);
        else if(loglvl == "warn")
            spdlog::set_level(spdlog::level::warn);
        else if(loglvl == "info")
            spdlog::set_level(spdlog::level::info);
        else
            spdlog::set_level(spdlog::level::debug);

        m_stAgentCfg.weburi = config["websocketurl"].as<std::string>();
        m_stAgentCfg.websocketOn = config["websocketOn"].as<bool>();
        m_stAgentCfg.websocketPort = config["websocketPort"].as<int>();

        m_stAgentCfg.detection = config["detection"].as<bool>();
        m_stAgentCfg.usedCamNum = config["USED_CAMERA_NUM"].as<int>();

        auto camDispIdx = config["camDispIdx"].as<string>();
        if("a" == camDispIdx)
        {
            m_stAgentCfg.camDispIdx = -1;
        }
        else
        {
            try{
                m_stAgentCfg.camDispIdx = std::stoi(camDispIdx);
            }
            catch(std::invalid_argument&){
                spdlog::warn("invalid camDispIdx");
                m_stAgentCfg.camDispIdx = 1;
            }
            
            // ymlCameraCfg.outPutWidth = ymlCameraCfg.undistoredWidth = 1920;
            // ymlCameraCfg.outPutHeight = ymlCameraCfg.undistoredHeight = 1080;
        }

    }
    catch (YAML::ParserException &ex) {
            spdlog::critical("stitcher cfg yaml:{}  parse failed:{}", stitcherCfgPath, ex.what());
            return RET_ERR;
    } catch (YAML::BadFile &ex) {
        spdlog::critical("stitcher cfg yaml:{} load failed:{}", stitcherCfgPath, ex.what());
        return RET_ERR;
    }
    // catch(...)
    // {
    //     spdlog::critical("yml parse failed, check your config yaml!");
    //     return RET_ERR;
    // }

    return RET_OK;
}

CameraDispAgent::CameraDispAgent():AgentBase()
{

}

CameraDispAgent::~CameraDispAgent()
{

}

int CameraDispAgent::init(const std::string cfgPath)
{
    return AgentBase::init(cfgPath);
}

CameraDispAgent2X::CameraDispAgent2X():CameraDispAgent()
{

}

CameraDispAgent2X::~CameraDispAgent2X()
{
    
}

int CameraDispAgent2X::init(const std::string cfgPath)
{
    return CameraDispAgent::init(cfgPath);
}

int CameraDispAgent2X::run()
{
    std::vector<cv::Mat> imgs(2);
    cv::Mat ret;
    while(1)
    {
        if(m_stAgentCfg.camDispIdx == -1)
        {
            m_apCameras[0]->getFrame(imgs[0], false);
            m_apCameras[1]->getFrame(imgs[1], false);
            cv::hconcat(std::vector<cv::Mat>{imgs[0], imgs[1]}, ret);
        }
        else
        {
            m_apCameras[m_stAgentCfg.camDispIdx-1]->getFrame(ret);
        }
        
        m_pRender->render(ret);
    }

}

CameraDispAgent4X::CameraDispAgent4X()
{

}

CameraDispAgent4X::~CameraDispAgent4X()
{

}

int CameraDispAgent4X::init(const std::string cfgPath)
{
    return CameraDispAgent::init(cfgPath);
}

// modify
int CameraDispAgent4X::run()
{
    std::vector<cv::Mat> imgs(2);
    cv::Mat ret;
    while(1)
    {
        m_apCameras[0]->getFrame(imgs[0], false);
        m_apCameras[1]->getFrame(imgs[1], false);

        cv::hconcat(std::vector<cv::Mat>{imgs[0], imgs[1]}, ret);
        m_pRender->render(ret);
    }

}