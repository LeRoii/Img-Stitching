#include "panocam.h"
#include "nvcam.hpp"
#include "stitcherconfig.h"
#include "PracticalSocket.h"
#include "spdlog/spdlog.h"

static unsigned short servPort = 10001;
static UDPSocket sock(servPort);
static char buffer[SLAVE_PCIE_UDP_BUF_LEN];

std::vector<cv::Mat> upImgs(4);
std::vector<cv::Mat> downImgs(4);

int  serverCap()
{
    downImgs.clear();
    int recvMsgSize; // Size of received message
    string sourceAddress; // Address of datagram source
    unsigned short sourcePort; // Port of datagram source
    cv::Mat recvedFrame;

    do {
        recvMsgSize = sock.recvFrom(buffer, SLAVE_PCIE_UDP_BUF_LEN, sourceAddress, sourcePort);
    } while (recvMsgSize > sizeof(int));
    int total_pack = ((int * ) buffer)[0];

    spdlog::info("expecting length of packs: {}", total_pack);
    char * longbuf = new char[SLAVE_PCIE_UDP_PACK_SIZE * total_pack];
    for (int i = 0; i < total_pack; i++) {
        recvMsgSize = sock.recvFrom(buffer, SLAVE_PCIE_UDP_BUF_LEN, sourceAddress, sourcePort);
        if (recvMsgSize != SLAVE_PCIE_UDP_PACK_SIZE) {
            spdlog::warn("Received unexpected size pack: {}", recvMsgSize);
            free(longbuf);
            return RET_ERR;
        }
        memcpy( & longbuf[i * SLAVE_PCIE_UDP_PACK_SIZE], buffer, SLAVE_PCIE_UDP_PACK_SIZE);
    }

    spdlog::debug("Received packet from {}:{}", sourceAddress, sourcePort);

    cv::Mat rawData = cv::Mat(1, SLAVE_PCIE_UDP_PACK_SIZE * total_pack, CV_8UC1, longbuf);
    recvedFrame = imdecode(rawData, cv::IMREAD_COLOR);
    spdlog::debug("size:[{},{}]", recvedFrame.size().width, recvedFrame.size().height);
    if (recvedFrame.size().width == 0) {
        spdlog::warn("decode failure!");
        // continue;
    }
    downImgs[2] = recvedFrame(cv::Rect(0,0,stitcherinputWidth, stitcherinputHeight)).clone();
    downImgs[3] = recvedFrame(cv::Rect(stitcherinputWidth,0,stitcherinputWidth, stitcherinputHeight)).clone();
    // imwrite("7.png", downImgs[2]);
    // imwrite("8.png", downImgs[3]);
    // imshow("recv", recvedFrame);
    // waitKey(1);
    free(longbuf);

    return RET_OK;
}

class panocam::panocamimpl
{
public:
    panocamimpl()
    {
        stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,1,"/dev/video0"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,2,"/dev/video1"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,3,"/dev/video2"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,4,"/dev/video3"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,5,"/dev/video4"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,6,"/dev/video5"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,7,"/dev/video7"},
                                        stCamCfg{camSrcWidth,camSrcHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,8,"/dev/video6"}};

        for(int i=0;i<USED_CAMERA_NUM;i++)
            cameras[i].reset(new nvCam(camcfgs[i]));

    }
    
    ~panocamimpl() = default;

    int init()
    {
        spdlog::info("init");
                
        return RET_OK;
    }

    int getCamFrame(int id, cv::Mat &frame)
    {
        if(id < 1 || id > 8)
        {
            spdlog::critical("invalid camera id");
            return RET_ERR;
        }
        if(id < 7)
        {
            return cameras[id-1]->getSrcFrame(frame);
        }
        else
        {
            while(serverCap() != RET_OK);
            frame = downImgs[id-5].clone();
        }
        return RET_OK;
    }

private:
    std::shared_ptr<nvCam> cameras[USED_CAMERA_NUM];
};

panocam::panocam():pimpl{std::make_unique<panocamimpl>()}
{

}

panocam::~panocam() = default;

int panocam::init()
{
    pimpl->init();
}

int panocam::getCamFrame(int id, cv::Mat &frame)
{
    return pimpl->getCamFrame(id, frame);
}