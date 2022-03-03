#ifndef _STITCHERCONFIG_H_
#define _STITCHERCONFIG_H_

#include <string>

const int SLAVE_PCIE_UDP_PACK_SIZE = 60000;
// const int SLAVE_PCIE_UDP_PACK_SIZE = 10000;

int vendor = 0;
int camSrcWidth = 3840;
int camSrcHeight = 2160;

int distorWidth =  1920;
int distorHeight = 1080;

int undistorWidth =  960;
int undistorHeight = 540;

int stitcherinputWidth = 640;
int stitcherinputHeight = 360;

const int RENDER_EGL = 0;
const int RENDER_OCV = 1;

int renderWidth = 1920;
int renderHeight = 1080;
int renderX = 0;
int renderY = 0;
int renderMode = 0;

// output render buffer, in general it's fixed
int renderBufWidth = 1920; 
int renderBufHeight = 1080;

const int CAMERA_NUM = 8;
int USED_CAMERA_NUM = 8;
const int SLAVE_PCIE_UDP_BUF_LEN = 65540;

const int RET_OK = 0;
const int RET_ERR = -1;

bool undistor = false;

float stitcherMatchConf = 0.3;
float stitcherAdjusterConf = 0.7f;
float stitcherBlenderStrength = 3;
float stitcherCameraExThres = 30;
float stitcherCameraInThres = 100;

int batchSize = 1;
int initMode = 1;

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