#ifndef _STITCHERCONFIG_H_
#define _STITCHERCONFIG_H_

const int SLAVE_PCIE_UDP_PACK_SIZE = 60000;
// const int SLAVE_PCIE_UDP_PACK_SIZE = 10000;

int camSrcWidth = 3840;
int camSrcHeight = 2160;

int distorWidth =  1920/2;
int distorHeight = 1080/2;

int undistorWidth =  1920/2;
int undistorHeight = 1080/2;

int stitcherinputWidth = 1920/4;
int stitcherinputHeight = 1080/4;

int renderWidth = 1920;
int renderHeight = 1080;
int renderX = 0;
int renderY = 0;

// in general it's fixed
int renderBufWidth = 1920; 
int renderBufHeight = 1080;

const int CAMERA_NUM = 8;
int USED_CAMERA_NUM = 6;
const int SLAVE_PCIE_UDP_BUF_LEN = 65540;

const int RET_OK = 0;
const int RET_ERR = -1;

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
	int id;
    char name[20];
};



#endif