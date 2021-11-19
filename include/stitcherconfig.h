#ifndef _STITCHERCONFIG_H_
#define _STITCHERCONFIG_H_

const int SLAVE_PCIE_UDP_PACK_SIZE = 60000;
// const int SLAVE_PCIE_UDP_PACK_SIZE = 10000;

const int camSrcWidth = 3840;
const int camSrcHeight = 2160;

const int undistorWidth =  1920/2;
const int undistorHeight = 1080/2;

const int stitcherinputWidth = 1920/4;
const int stitcherinputHeight = 1080/4;

// int stitcherinputWidth = 1920/2;
// int stitcherinputHeight = 1080/2;

const int CAMERA_NUM = 8;
const int USED_CAMERA_NUM = 6;
const int SLAVE_PCIE_UDP_BUF_LEN = 65540;

const int RET_OK = 0;
const int RET_ERR = -1;

struct stCamCfg
{
    int camSrcWidth;
    int camSrcHeight;
	int distoredWidth;
	int distoredHeight;
	int retWidth;
	int retHeight;
	int id;
    char name[20];
};



#endif