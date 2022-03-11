#ifndef _STITCHERCONFIG_H_
#define _STITCHERCONFIG_H_

#include <string>

const int SLAVE_PCIE_UDP_PACK_SIZE = 60000;
// const int SLAVE_PCIE_UDP_PACK_SIZE = 10000;
const int RENDER_EGL = 0;
const int RENDER_OCV = 1;
const int CAMERA_NUM = 8;
const int SLAVE_PCIE_UDP_BUF_LEN = 65540;
const int RET_OK = 0;
const int RET_ERR = -1;

extern int vendor;
extern int camSrcWidth;
extern int camSrcHeight;

extern int distorWidth;
extern int distorHeight;

extern int undistorWidth;
extern int undistorHeight;

extern int stitcherinputWidth;
extern int stitcherinputHeight;

extern int renderWidth;
extern int renderHeight;
extern int renderX;
extern int renderY;
extern int renderMode;

// output render buffer, in general it's fixed
extern int renderBufWidth; 
extern int renderBufHeight;

extern int USED_CAMERA_NUM;
extern bool undistor;

extern float stitcherMatchConf;
extern float stitcherAdjusterConf;
extern float stitcherBlenderStrength;
extern float stitcherCameraExThres;
extern float stitcherCameraInThres;

extern int batchSize;
extern int initMode;

struct stCamCfg
{
    int camSrcWidth;
    int camSrcHeight;
	int distoredWidth;
	int distoredHeight;
	int undistoredWidth;
	int undistoredHeight;
	int retWidth;
	int retHeight;
    bool undistor;
	int id;
    char name[20];
    int vendor;
};

struct nvrenderCfg
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
    int id;
    float matchConf;
    float adjusterConf;
    float blendStrength;
    float stitchercameraExThres;
    float stitchercameraInThres;
    std::string cfgPath;
};

enum enStitcherInitMode
{
    enInitALL = 1,
    enInitByDefault = 2,
    enInitByCfg = 3
};

#endif