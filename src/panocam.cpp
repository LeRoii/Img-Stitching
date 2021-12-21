#include "panocam.h"
#include "yaml-cpp/yaml.h"
#include "nvcam.hpp"
#include "ocvstitcher.hpp"
#include "stitcherconfig.h"
#include "PracticalSocket.h"
#include "imageProcess.h"
#include "spdlog/spdlog.h"


static unsigned short servPort = 10001;
static UDPSocket sock(servPort);
static char buffer[SLAVE_PCIE_UDP_BUF_LEN];
// static const int USED_CAMERA_NUM = 6;

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
    panocamimpl(std::string yamlpath)
    {
        YAML::Node config = YAML::LoadFile(yamlpath);
        int camw = config["camwidth"].as<int>();
        int camh = config["camheight"].as<int>();
        std::string net = config["netpath"].as<string>();
        std::string camcfg = config["camcfgpath"].as<string>();
        std::string canname = config["canname"].as<string>();

        stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camw,camh,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,1,"/dev/video0"},
                                        stCamCfg{camw,camh,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,2,"/dev/video1"},
                                        stCamCfg{camw,camh,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,3,"/dev/video2"},
                                        stCamCfg{camw,camh,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,4,"/dev/video3"},
                                        stCamCfg{camw,camh,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,5,"/dev/video4"},
                                        stCamCfg{camw,camh,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,6,"/dev/video5"},
                                        stCamCfg{camw,camh,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,7,"/dev/video7"},
                                        stCamCfg{camw,camh,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,8,"/dev/video6"}};

        for(int i=0;i<USED_CAMERA_NUM;i++)
            cameras[i].reset(new nvCam(camcfgs[i]));


        stitchers[0].reset(new ocvStitcher(stitcherinputWidth, stitcherinputHeight, 1, camcfg));
        stitchers[1].reset(new ocvStitcher(stitcherinputWidth, stitcherinputHeight, 2, camcfg));

        pImgProc = new imageProcessor(net, canname);
        spdlog::debug("panocam ctor complete");
    }
    
    ~panocamimpl() = default;

    int init(enInitMode mode)
    {
        spdlog::info("init");
        bool initonlie = ((mode == INIT_ONLINE) ? true : false);
        upImgs.clear();
        int failnum = 0;
        do{
            upImgs.clear();
            for(int i=0;i<4;i++)
            {
                cameras[i]->read_frame();
                upImgs.push_back(cameras[i]->m_ret);
            }
            failnum++;
            if(failnum > 5)
            {
                spdlog::critical("initalization failed due to environment");
                return RET_ERR;
            }
        }
        while(stitchers[0]->init(upImgs, initonlie) != 0); 
        spdlog::info("init completed 50%");

        failnum = 0;
        do{
            serverCap();
            cameras[4]->read_frame();
            cameras[5]->read_frame();
            downImgs[0] = cameras[4]->m_ret;
            downImgs[1] = cameras[5]->m_ret;
            failnum++;
            if(failnum > 5)
            {
                spdlog::critical("initalization failed due to environment");
                return RET_ERR;
            }
        }
        while(stitchers[1]->init(downImgs, initonlie) != 0);

        std::vector<std::thread> threads;
        for(int i=0;i<USED_CAMERA_NUM;i++)
            threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
        for(auto& th:threads)
            th.detach();

        spdlog::info("init completed!");
                
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

    int getPanoFrame(cv::Mat &ret)
    {
        auto all = cv::getTickCount();
        std::thread server(serverCap);
        cameras[0]->getFrame(upImgs[0]);
        cameras[1]->getFrame(upImgs[1]);
        cameras[2]->getFrame(upImgs[2]);
        cameras[3]->getFrame(upImgs[3]);
        cameras[4]->getFrame(downImgs[0]);
        cameras[5]->getFrame(downImgs[1]);
        server.join();
        spdlog::debug("imgs cap fini");
        cv::Mat up, down, rett;
        stitchers[0]->process(upImgs, up);
        stitchers[1]->process(downImgs, down);

        int width = min(up.size().width, down.size().width);
        int height = min(up.size().height, down.size().height) - 30;
        up = up(Rect(0,15,width,height));
        down = down(Rect(0,15,width,height));

        cv::vconcat(up, down, ret);
        cv::rectangle(ret, cv::Rect(0, height - 2, width, 4), cv::Scalar(0,0,0), -1, 1, 0);
        spdlog::info("******all takes: {:03.3f} ms", ((getTickCount() - all) / getTickFrequency()) * 1000);

    }

    int detect(cv::Mat &img, std::vector<int> &ret)
    {
        img = pImgProc->ImageDetect(img, ret);

        return RET_OK;
    }

    int imgEnhancement(cv::Mat &img)
    {
        img = pImgProc->SSR(img);

        return RET_OK;
    }
    

private:
    std::shared_ptr<nvCam> cameras[CAMERA_NUM];
    std::shared_ptr<ocvStitcher> stitchers[2];
    imageProcessor *pImgProc; 
};

panocam::panocam(std::string yamlpath):
    pimpl{std::make_unique<panocamimpl>(yamlpath)}
{

}

panocam::~panocam() = default;

int panocam::init(enInitMode mode)
{
    return pimpl->init(mode);
}

int panocam::getCamFrame(int id, cv::Mat &frame)
{
    return pimpl->getCamFrame(id, frame);
}

int panocam::getPanoFrame(cv::Mat &ret)
{
    return pimpl->getPanoFrame(ret);
}

int panocam::detect(cv::Mat &img, std::vector<int> &ret)
{
    return pimpl->detect(img, ret);
}

int panocam::imgEnhancement(cv::Mat &img)
{
    return pimpl->imgEnhancement(img);
}