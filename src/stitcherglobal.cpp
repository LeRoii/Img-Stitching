
#include <string>

std::string UDP_PORT = "9000";
std::string UDP_SERVADD = "192.168.1.120";

//208
// int vendor = 0;
// int camSrcWidth = 3840;
// int camSrcHeight = 2160;

// int distorWidth =  1920;
// int distorHeight = 1080;

// int undistorWidth =  1920;
// int undistorHeight = 1080;

// int stitcherinputWidth = 640;
// int stitcherinputHeight = 360;

// int renderWidth = 1920;
// int renderHeight = 1080;
// int renderX = 0;
// int renderY = 0;
// int renderMode = 0;

// // output render buffer, in general it's fixed
// int renderBufWidth = 1920; 
// int renderBufHeight = 1080;

// int USED_CAMERA_NUM = 8;

// bool undistor = false;

// float stitcherMatchConf = 0.3;
// float stitcherAdjusterConf = 0.7f;
// float stitcherBlenderStrength = 3;
// float stitcherCameraExThres = 30;
// float stitcherCameraInThres = 100;

// int batchSize = 1;
// int initMode = 1;


//207
int vendor = 0;
int camSrcWidth = 1920;
int camSrcHeight = 1080;

int distorWidth =  1920;
int distorHeight = 1080;

int undistorWidth =  960;
int undistorHeight = 540;

int stitcherinputWidth = 640;
int stitcherinputHeight = 360;

int renderWidth = 1920;
int renderHeight = 1080;
int renderX = 0;
int renderY = 0;
int renderMode = 0;

// output render buffer, in general it's fixed
int renderBufWidth = 1920; 
int renderBufHeight = 1080;

int USED_CAMERA_NUM = 8;
short int num_images = 4;

bool undistor = false;

float stitcherMatchConf = 0.3;
float stitcherAdjusterConf = 0.7f;
float stitcherBlenderStrength = 3;
float stitcherCameraExThres = 30;
float stitcherCameraInThres = 100;

int batchSize = 1;
int initMode = 1;
