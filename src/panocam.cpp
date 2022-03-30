#include "stitcherglobal.h"
#include "yaml-cpp/yaml.h"
#include "panocam.h"

#include "nvcam.hpp"

#include "ocvstitcher.hpp"
#include "PracticalSocket.h"
#include "imageProcess.h"
#include "spdlog/spdlog.h"
#include "nvrender.h"


std::vector<cv::Mat> upImgs(4);
std::vector<cv::Mat> downImgs(4);
#if CAM_IMX424

static unsigned short servPort = 10001;
static UDPSocket sock(servPort);
static char buffer[SLAVE_PCIE_UDP_BUF_LEN];

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
#endif

static int verify()
{
    int                 sockfd;
    struct ifreq        ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket error");
        exit(1);
    }
    strncpy(ifr.ifr_name, "eth1", IFNAMSIZ);      //Interface name

    char * buf = new char[6];

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == 0) {  //SIOCGIFHWADDR 获取hardware address
        memcpy(buf, ifr.ifr_hwaddr.sa_data, 6);
    }
    // printf("mac:%02x:%02x:%02x:%02x:%02x:%02x\n", buf[0]&0xff, buf[1]&0xff, buf[2]&0xff, buf[3]&0xff, buf[4]&0xff, buf[5]&0xff);

    
    // char gt[] = "00:54:5a:19:03:5f";//91v-dev
    // char gt[] = "00:54:5a:1b:02:7b";//91s-dev
    char gt[] = "00:54:5a:1c:00:bd";//91s-207
    
    char p[50];
    sprintf(p, "%02x:%02x:%02x:%02x:%02x:%02x", buf[0]&0xff, buf[1]&0xff, buf[2]&0xff, buf[3]&0xff, buf[4]&0xff, buf[5]&0xff);
    // printf("p::%s\n", p);

    return strcmp(gt, p);
}

class panocam::panocamimpl
{
public:
    panocamimpl(std::string yamlpath):framecnt(0)
    {
        YAML::Node config = YAML::LoadFile(yamlpath);
        int camw = config["camwidth"].as<int>();
        int camh = config["camheight"].as<int>();
        std::string net = config["netpath"].as<string>();
        std::string canname = config["canname"].as<string>();
        std::string camcfg = "";

        renderWidth = config["renderWidth"].as<int>();
        renderHeight = config["renderHeight"].as<int>();
        renderX = config["renderX"].as<int>();
        renderY = config["renderY"].as<int>();

        stitcherBlenderStrength = config["quality"].as<float>();
        initMode = config["initMode"].as<int>();

        std::string loglvl = config["loglvl"].as<string>();
        if(loglvl == "critical-iair")
            spdlog::set_level(spdlog::level::critical);
        else if(loglvl == "trace-iair")
            spdlog::set_level(spdlog::level::trace);
        else if(loglvl == "warn-iair")
            spdlog::set_level(spdlog::level::warn);
        else if(loglvl == "info")
            spdlog::set_level(spdlog::level::info);
        else if(loglvl == "debug-iair")
            spdlog::set_level(spdlog::level::debug);
        else
            spdlog::set_level(spdlog::level::info);
#if CAM_IMX424
        USED_CAMERA_NUM = 6;
#endif

        stCamCfg camcfgs[CAMERA_NUM] = {stCamCfg{camw,camh,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,1,"/dev/video0", vendor},
                                    stCamCfg{camw,camh,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,2,"/dev/video1", vendor},
                                    stCamCfg{camw,camh,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,3,"/dev/video2", vendor},
                                    stCamCfg{camw,camh,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,4,"/dev/video3", vendor},
                                    stCamCfg{camw,camh,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,5,"/dev/video4", vendor},
                                    stCamCfg{camw,camh,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,6,"/dev/video5", vendor},
                                    stCamCfg{camw,camh,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,7,"/dev/video6", vendor},
                                    stCamCfg{camw,camh,distorWidth,distorHeight,undistorWidth,undistorHeight,stitcherinputWidth,stitcherinputHeight,undistor,8,"/dev/video7", vendor}};

        for(int i=0;i<USED_CAMERA_NUM;i++)
            cameras[i].reset(new nvCam(camcfgs[i]));

        stStitcherCfg stitchercfg[2] = {stStitcherCfg{stitcherinputWidth, stitcherinputHeight, 1, stitcherMatchConf, stitcherAdjusterConf, stitcherBlenderStrength, stitcherCameraExThres, stitcherCameraInThres, camcfg},
                                    stStitcherCfg{stitcherinputWidth, stitcherinputHeight, 2, stitcherMatchConf, stitcherAdjusterConf, stitcherBlenderStrength, stitcherCameraExThres, stitcherCameraInThres, camcfg}};

        stitchers[0].reset(new ocvStitcher(stitchercfg[0]));
        stitchers[1].reset(new ocvStitcher(stitchercfg[1]));

        pImgProc = new imageProcessor(net, canname, batchSize);

        nvrenderCfg rendercfg{renderBufWidth, renderBufHeight, renderWidth, renderHeight, renderX, renderY, renderMode};
        // pRenderer = new nvrender(rendercfg);

            
        if(stitcherinputWidth == 480)
            finalcut = 15;
        else if(stitcherinputWidth == 640)
            finalcut = 40;

        spdlog::debug("panocam ctor complete");
    }
    
    ~panocamimpl() = default;

    int init()
    {
        spdlog::info("panocam init start");

#ifndef DEV_MODE
        if(verify())
        {
            spdlog::critical("verification failed, exit");
            return RET_ERR;
        }
#endif

        if(!(initMode == 1 || initMode == 2))
        {
            spdlog::critical("invalid init mode, exit");
            return RET_ERR;
        }
        // bool initonlie = ((mode == INIT_ONLINE) ? true : false);
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
        while(stitchers[0]->init(upImgs, initMode) != 0); 
        spdlog::info("init completed 50%");

        failnum = 0;
        do{
#if CAM_IMX390
            downImgs.clear();
            for(int i=0;i<4;i++)
            {
                cameras[i+4]->read_frame();
                downImgs.push_back(cameras[i+4]->m_ret);
            }
#elif CAM_IMX424
            serverCap();
            cameras[4]->read_frame();
            cameras[5]->read_frame();
            downImgs[0] = cameras[4]->m_ret;
            downImgs[1] = cameras[5]->m_ret;
#endif
            failnum++;
            if(failnum > 5)
            {
                spdlog::critical("initalization failed due to environment");
                return RET_ERR;
            }
        }
        while(stitchers[1]->init(downImgs, initMode) != 0);

        spdlog::info("init completed!");

        std::vector<std::thread> threads;
        for(int i=0;i<USED_CAMERA_NUM;i++)
            threads.push_back(std::thread(&nvCam::run, cameras[i].get()));
        for(auto& th:threads)
            th.detach();
                
        return RET_OK;
    }

    int getCamFrame(int id, cv::Mat &frame)
    {
        if(id < 1 || id > 8)
        {
            spdlog::critical("invalid camera id");
            return RET_ERR;
        }
#if CAM_IMX424
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
#elif CAM_IMX390
        return cameras[id-1]->getSrcFrame(frame);
#endif
        
    }

    int getPanoFrame(cv::Mat &ret)
    {
#if CAM_IMX424
        std::thread server(serverCap);
        cameras[0]->getFrame(upImgs[0]);
        cameras[1]->getFrame(upImgs[1]);
        cameras[2]->getFrame(upImgs[2]);
        cameras[3]->getFrame(upImgs[3]);
        cameras[4]->getFrame(downImgs[0]);
        cameras[5]->getFrame(downImgs[1]);
        server.join();
#elif CAM_IMX390
        cameras[0]->getFrame(upImgs[0]);
        cameras[1]->getFrame(upImgs[1]);
        cameras[2]->getFrame(upImgs[2]);
        cameras[3]->getFrame(upImgs[3]);
        cameras[4]->getFrame(downImgs[0]);
        cameras[5]->getFrame(downImgs[1]);
        cameras[6]->getFrame(downImgs[2]);
        cameras[7]->getFrame(downImgs[3]);
#endif

        spdlog::debug("imgs cap fini");
        cv::Mat up, down, rett;
        // stitchers[0]->process(upImgs, up);
        // stitchers[1]->process(downImgs, down);

        std::thread t1 = std::thread(&ocvStitcher::process, stitchers[0], std::ref(upImgs), std::ref(up));
        std::thread t2 = std::thread(&ocvStitcher::process, stitchers[1], std::ref(downImgs), std::ref(down));

        t1.join();
        t2.join();

        // cv::flip(down, down, 1);
        int width = min(up.size().width, down.size().width);
        int height = min(up.size().height, down.size().height) - finalcut*2;
        up = up(Rect(0,finalcut,width,height));
        down = down(Rect(0,finalcut,width,height));

        cv::vconcat(up, down, ret);
        cv::rectangle(ret, cv::Rect(0, height - 2, width, 4), cv::Scalar(0,0,0), -1, 1, 0);

        spdlog::debug("panorama frame:{}",framecnt++);

        return RET_OK;
    }

    int detect(cv::Mat &img, std::vector<int> &ret)
    {
        if(img.empty())
        {
            spdlog::critical("img is empty! exit");
            return RET_ERR;
        }
        img = pImgProc->ImageDetect(img, ret);

        return RET_OK;
    }

    int detect(cv::Mat &img)
    {
        if(img.empty())
        {
            spdlog::critical("img is empty! exit");
            return RET_ERR;
        }
        img = pImgProc->ProcessOnce(img);

        return RET_OK;
    }

    int imgEnhancement(cv::Mat &img)
    {
        if(img.empty())
        {
            spdlog::critical("img is empty! exit");
            return RET_ERR;
        }
        img = pImgProc->SSR(img);

        return RET_OK;
    }

    int render(cv::Mat &img)
    {
        if(img.empty())
        {
            spdlog::critical("img is empty! exit");
            return RET_ERR;
        }
        pRenderer->render(img);

        return RET_OK;
    }

    int drawCross(cv::Mat &img)
    {
        int w = img.cols;
        int h = img.rows;
        int x1 = w/2;
        int y1 = h/4;
        int y2 = h/4*3;
        
        cv::line(img, cv::Point(x1-10,y1), cv::Point(x1+10,y1), cv::Scalar(0,255,0), 2);
        cv::line(img, cv::Point(x1,y1-10), cv::Point(x1,y1+10), cv::Scalar(0,255,0), 2);

        cv::line(img, cv::Point(x1-10,y2), cv::Point(x1+10,y2), cv::Scalar(0,255,0), 2);
        cv::line(img, cv::Point(x1,y2-10), cv::Point(x1,y2+10), cv::Scalar(0,255,0), 2);
    }
    

private:
    std::shared_ptr<nvCam> cameras[CAMERA_NUM];
    std::shared_ptr<ocvStitcher> stitchers[2];
    imageProcessor *pImgProc; 
    nvrender *pRenderer;
    int finalcut;
    int framecnt;
};

panocam::panocam(std::string yamlpath):
    pimpl{std::make_unique<panocamimpl>(yamlpath)}
{
    // nvrenderCfg rendercfg{1920, 1080, 1920/2, 1080/2, 0, 0};
    // pRenderer = new nvrender(rendercfg);

}

panocam::~panocam() = default;

int panocam::init()
{
    return pimpl->init();
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

int panocam::detect(cv::Mat &img)
{
    return pimpl->detect(img);
}

int panocam::imgEnhancement(cv::Mat &img)
{
    return pimpl->imgEnhancement(img);
}

int panocam::render(cv::Mat &img)
{
    return pimpl->render(img);
}

int panocam::drawCross(cv::Mat &img)
{
    return pimpl->drawCross(img);
}

int panocam::verify()
{
    int                 sockfd;
    struct ifreq        ifr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket error");
        exit(1);
    }
    strncpy(ifr.ifr_name, "eth1", IFNAMSIZ);      //Interface name

    char * buf = new char[6];

    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == 0) {  //SIOCGIFHWADDR 获取hardware address
        memcpy(buf, ifr.ifr_hwaddr.sa_data, 6);
    }
    // printf("mac:%02x:%02x:%02x:%02x:%02x:%02x\n", buf[0]&0xff, buf[1]&0xff, buf[2]&0xff, buf[3]&0xff, buf[4]&0xff, buf[5]&0xff);

    
    char gt[] = "00:54:5a:19:03:5f";//91v-dev
    // char gt[] = "00:54:5a:1b:02:7b";//91s-dev
    // char gt[] = "00:54:5a:1c:00:bd";//91s-207
    
    char p[50];
    sprintf(p, "%02x:%02x:%02x:%02x:%02x:%02x", buf[0]&0xff, buf[1]&0xff, buf[2]&0xff, buf[3]&0xff, buf[4]&0xff, buf[5]&0xff);
    // printf("p::%s\n", p);

    return strcmp(gt, p);
}