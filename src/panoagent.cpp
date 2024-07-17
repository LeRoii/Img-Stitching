#include "agentfactory.h"

#ifdef DEV_MODE
static std::string stitchercfgpath = "../cfg/stitcher-imx390cfg.yaml";
#else
static std::string stitchercfgpath = "/etc/panorama/stitcher-imx390cfg.yaml";
#endif

int main()
{
    CameraDispAgentFactory *camDispFactory = new CameraDispAgentFactory();
    camDispFactory->init(stitchercfgpath);
    AgentBase *pCamDisp = camDispFactory->Create();
    pCamDisp->init(stitchercfgpath);
    pCamDisp->run();

    return 0;
}