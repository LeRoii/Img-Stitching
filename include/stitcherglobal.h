#ifndef _STITCHERCONFIG_H_
#define _STITCHERCONFIG_H_

#include <string>

const int SLAVE_PCIE_UDP_PACK_SIZE = 60000;
// const int SLAVE_PCIE_UDP_PACK_SIZE = 10000;
const int RENDER_EGL = 0;
const int RENDER_OCV = 1;
const int RENDER_NONE = 2;
const int CAMERA_NUM = 8;
const int SLAVE_PCIE_UDP_BUF_LEN = 65540;
const int RET_OK = 0;
const int RET_ERR = -1;

const unsigned char STATUS_OK = 0;
const unsigned char STATUS_VERIFICATION_FAILED = 0xE0;
const unsigned char STATUS_INITALIZATION_FAILED = 0xE1;

extern int vendor;
extern short int num_images;

extern int renderWidth;
extern int renderHeight;
extern int renderX;
extern int renderY;
extern int renderMode;

extern std::string UDP_PORT;
extern std::string UDP_SERVADD;
extern std::string weburi;

// output render buffer, in general it's fixed
extern int renderBufWidth; 
extern int renderBufHeight;

extern int USED_CAMERA_NUM;

struct stCamCfg
{
    int camSrcWidth;
    int camSrcHeight;
	int distoredWidth;
	int distoredHeight;
	int undistoredWidth;
	int undistoredHeight;
	int outPutWidth;
	int outPutHeight;
    bool undistor;
	int id;
    char name[20];
    std::string vendor;
    std::string sensor;
    int fov;
};

struct stNvrenderCfg
{
    int bufferw;
    int bufferh;
    int renderw;
    int renderh;
    int renderx;
    int rendery;
    int mode;//0 for egl, 1 for opencv
};

struct stStitcherCfg
{
    int width;
    int height;
    short int id;
    short int num_images;
    float matchConf;
    float adjusterConf;
    float blendStrength;
    float stitchercameraExThres;
    float stitchercameraInThres;
    std::string cfgPath;
    int initMode;
};

struct stSysStatus
{
    uint8_t deviceStatus;
    uint8_t cameraStatus;
    bool zoomTrigger;
    bool detectionTrigger;
    bool enhancementTrigger;
    bool crossTrigger;
    bool saveTrigger;
    uint8_t displayMode;
    int zoomPointX;
    int zoomPointY;
    stSysStatus():deviceStatus(0),cameraStatus(0xff),zoomTrigger(false),crossTrigger(false), \
        saveTrigger(false),detectionTrigger(false),enhancementTrigger(false),displayMode(0xCA){}
};

struct stAgentCfg
{
    std::string weburi;
    bool websocketOn;
    int websocketPort;
    bool detection;
    bool imgEnhancement;
    int usedCamNum;
    int camDispIdx;
};

enum enStitcherInitMode
{
    enInitALL = 1,
    enInitByDefault = 2,
    enInitByCfg = 3
};

#endif