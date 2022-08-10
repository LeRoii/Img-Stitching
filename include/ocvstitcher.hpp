#ifndef _OCVSTITCHER_HPP_
#define _OCVSTITCHER_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include "opencv2/opencv_modules.hpp"
#include <opencv2/core/utility.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/stitching/detail/autocalib.hpp"
#include "opencv2/stitching/detail/blenders.hpp"
#include "opencv2/stitching/detail/timelapsers.hpp"
#include "opencv2/stitching/detail/camera.hpp"
#include "opencv2/stitching/detail/exposure_compensate.hpp"
#include "opencv2/stitching/detail/matchers.hpp"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "opencv2/stitching/detail/seam_finders.hpp"
#include "opencv2/stitching/detail/warpers.hpp"
#include "opencv2/stitching/warpers.hpp"

#include "spdlog/spdlog.h"
#include "stitcherglobal.h"

using namespace std;
using namespace cv;
using namespace cv::detail;

#define LOG(msg) std::cout << msg
#define LOGLN(msg) std::cout << msg << std::endl

//static int num_images = 4;

static std::mutex stmtx;

#if CENTRIC_STRUCT
// circle structure
// silver st
// static std::string camParams[2] = {"487.808,0,320,0,487.808,180,0,0,1,0.358901,-0.00255477,-0.933372,-0.00515675,0.999976,-0.00471995,0.933361,0.00650716,0.358879,\
// 533.62,0,320,0,533.62,180,0,0,1,0.927762,-0.00397522,-0.373152,0.0130791,0.999675,0.0218688,0.372944,-0.0251695,0.927512,\
// 566.215,0,320,0,566.215,180,0,0,1,0.920505,0.0103068,0.390594,-0.0137577,0.999887,0.00603792,-0.390488,-0.0109316,0.920543,\
// 595.578,0,320,0,595.578,180,0,0,1,0.401075,0.014492,0.915931,0.00593535,0.999813,-0.0184182,-0.916026,0.0128234,0.400914,\
// 549.917",
// "492.936,0,320,0,492.936,180,0,0,1,0.333934,0.00376102,-0.942589,-0.00390736,0.999989,0.00260578,0.942588,0.00281288,0.333945,\
// 531.666,0,320,0,531.666,180,0,0,1,0.920266,-0.00331873,-0.391279,-0.00138152,0.99993,-0.0117304,0.391291,0.0113356,0.920197,\
// 563.184,0,320,0,563.184,180,0,0,1,0.91131,0.00169592,0.411716,0.00588441,0.999836,-0.0171433,-0.411678,0.0180456,0.911151,\
// 605.804,0,320,0,605.804,180,0,0,1,0.386633,0.0122536,0.922152,-0.00720674,0.999921,-0.0102654,-0.922205,-0.00267676,0.386691,\
// 547.425"
// };

//258 green st
// static std::string camParams[2] = {"5093.54,0,320,0,5093.54,180,0,0,1,0.990251,-0.0289489,-0.13625,-0.0162866,0.947392,-0.31966,0.138336,0.318763,0.937685,\
// 5062.47,0,320,0,5062.47,180,0,0,1,0.998381,-0.0367373,-0.043432,0.0209781,0.947459,-0.319187,0.0528762,0.317759,0.946696,\
// 4976.91,0,320,0,4976.91,180,0,0,1,0.998971,0.0103404,0.04415,0.00423479,0.948122,-0.317878,-0.0451466,0.317738,0.947103,\
// 4947.03,0,320,0,4947.03,180,0,0,1,0.989246,0.05499,0.135532,-0.0091451,0.948075,-0.317916,-0.145977,0.313257,0.938382,\
// 5019.69",
// "6157.41,0,320,0,6157.41,180,0,0,1,0.993221,-0.0372395,-0.110116,0.0193385,0.98703,-0.159369,0.114622,0.15616,0.981059,\
// 6149.73,0,320,0,6149.73,180,0,0,1,0.999114,0.0213781,-0.0362632,-0.0269055,0.986826,-0.159535,0.0323749,0.160369,0.986526,\
// 6150.96,0,320,0,6150.96,180,0,0,1,0.999231,0.0132883,0.036889,-0.00720142,0.987013,-0.160476,-0.0385424,0.160087,0.98635,\
// 6156.61,0,320,0,6156.61,180,0,0,1,0.993984,0.00266958,0.10949,0.0149602,0.987023,-0.159879,-0.108496,0.160555,0.981046,\
// 6153.79"
// };

//4cam
// static std::string camParams[2] = {"217.261,0,320,0,217.261,180,0,0,1,-0.853292,-0.00614162,-0.521397,-0.0104169,0.999932,0.00526942,0.521329,0.0099277,-0.853298,\
// 238.693,0,320,0,238.693,180,0,0,1,0.550965,0.0204085,-0.834279,-0.00805931,0.999784,0.0191348,0.83449,-0.00381886,0.55101,\
// 238.385,0,320,0,238.385,180,0,0,1,0.880639,0.00399262,0.473772,-0.0096525,0.999908,0.00951537,-0.47369,-0.0129527,0.880596,\
// 229.537,0,320,0,229.537,180,0,0,1,-0.471243,0.0132046,0.881904,-0.00859877,0.999772,-0.0195641,-0.881961,-0.0168027,-0.471022,\
// 233.961",
// "217.261,0,320,0,217.261,180,0,0,1,-0.853292,-0.00614162,-0.521397,-0.0104169,0.999932,0.00526942,0.521329,0.0099277,-0.853298,\
// 238.693,0,320,0,238.693,180,0,0,1,0.550965,0.0204085,-0.834279,-0.00805931,0.999784,0.0191348,0.83449,-0.00381886,0.55101,\
// 238.385,0,320,0,238.385,180,0,0,1,0.880639,0.00399262,0.473772,-0.0096525,0.999908,0.00951537,-0.47369,-0.0129527,0.880596,\
// 229.537,0,320,0,229.537,180,0,0,1,-0.471243,0.0132046,0.881904,-0.00859877,0.999772,-0.0195641,-0.881961,-0.0168027,-0.471022,\
// 233.961"
// };


/*  sensing 640x380
static std::string camParams[2] = {"226.375,0,320,0,226.375,180,0,0,1,-0.987131,-0.0204767,-0.1586,-0.0138118,0.998979,-0.0430127,0.159319,-0.0402686,-0.986406,
222.303,0,320,0,222.303,180,0,0,1,0.206127,-0.0035511,-0.978519,-0.00716956,0.999961,-0.0051392,0.978499,0.00807487,0.206094,
230.421,0,320,0,230.421,180,0,0,1,0.988664,0.0165566,0.149227,-0.0133407,0.999657,-0.0225256,-0.149549,0.0202794,0.988546,
228.793,0,320,0,228.793,180,0,0,1,-0.141661,0.0632726,0.987891,-0.00729448,0.997861,-0.0649571,-0.989888,-0.016408,-0.140896,
227.584,
"527.008,0,320,0,527.008,180,0,0,1,0.30894,0.0124199,-0.951001,-0.0106465,0.999897,0.00959986,0.951022,0.00715901,0.30904,\
539.871,0,320,0,539.871,180,0,0,1,0.94793,-0.0269944,-0.317333,0.0255449,0.999636,-0.00872831,0.317453,0.000167585,0.948274,\
547.848,0,320,0,547.848,180,0,0,1,0.949426,0.0149656,0.313634,-0.0238263,0.999417,0.0244377,-0.313085,-0.0306745,0.94923,\
552.857,0,320,0,552.857,180,0,0,1,0.297381,0.0106519,0.9547,0.00570208,0.9999,-0.0129323,-0.954742,0.00928961,0.29729,\
543.86"
};
*/


// sensing 720x405
// static std::string camParams[2] = {"249.497,0,360,0,249.497,204,0,0,1,-0.619536,-0.0366182,-0.784113,-0.0193113,0.99932,-0.0314103,0.78473,-0.00431758,-0.619822,\
// 271.504,0,360,0,271.504,204,0,0,1,0.790702,0.0089529,-0.612136,-0.00312861,0.999939,0.0105835,0.612194,-0.00645328,0.790682,\
// 254.484,0,360,0,254.484,204,0,0,1,0.674464,0.0147844,0.73816,-0.0191019,0.999814,-0.00257147,-0.738061,-0.0123659,0.674621,\
// 249.509,0,360,0,249.509,204,0,0,1,-0.752344,0.0299483,0.65809,-0.00451026,0.998709,-0.0506055,-0.658755,-0.0410409,-0.751237,\
// 251.996",
// "527.008,0,320,0,527.008,180,0,0,1,0.30894,0.0124199,-0.951001,-0.0106465,0.999897,0.00959986,0.951022,0.00715901,0.30904,\
// 539.871,0,320,0,539.871,180,0,0,1,0.94793,-0.0269944,-0.317333,0.0255449,0.999636,-0.00872831,0.317453,0.000167585,0.948274,\
// 547.848,0,320,0,547.848,180,0,0,1,0.949426,0.0149656,0.313634,-0.0238263,0.999417,0.0244377,-0.313085,-0.0306745,0.94923,\
// 552.857,0,320,0,552.857,180,0,0,1,0.297381,0.0106519,0.9547,0.00570208,0.9999,-0.0129323,-0.954742,0.00928961,0.29729,\
// 543.86"
// };

// lj 120fov undistored 720x405
// static std::string camParams[2] = {"274.607,0,360,0,274.607,202.5,0,0,1,0.0989508,0.0389684,-0.994329,0.0212707,0.998922,0.0412651,0.994865,-0.0252333,0.0980151,\
// 264.726,0,360,0,264.726,202.5,0,0,1,0.997668,-0.0162984,0.0662799,0.0170039,0.999804,-0.0100948,-0.0661024,0.0111982,0.99775,\
// 272.31,0,360,0,272.31,202.5,0,0,1,-0.0610024,0.040629,0.99731,0.0213151,0.998996,-0.0393939,-0.99791,0.0188547,-0.0618072,\
// 261.052,0,360,0,261.052,202.5,0,0,1,-0.997442,0.0176576,-0.0692612,0.0178143,0.99984,-0.00164482,0.069221,-0.00287443,-0.997597,\
// 268.518",
// "527.008,0,320,0,527.008,180,0,0,1,0.30894,0.0124199,-0.951001,-0.0106465,0.999897,0.00959986,0.951022,0.00715901,0.30904,\
// 539.871,0,320,0,539.871,180,0,0,1,0.94793,-0.0269944,-0.317333,0.0255449,0.999636,-0.00872831,0.317453,0.000167585,0.948274,\
// 547.848,0,320,0,547.848,180,0,0,1,0.949426,0.0149656,0.313634,-0.0238263,0.999417,0.0244377,-0.313085,-0.0306745,0.94923,\
// 552.857,0,320,0,552.857,180,0,0,1,0.297381,0.0106519,0.9547,0.00570208,0.9999,-0.0129323,-0.954742,0.00928961,0.29729,\
// 543.86"

// lj 120fov distored 720x405
// static std::string camParams[2] = {"269.812,0,360,0,269.812,202.5,0,0,1,0.185945,0.0363414,-0.981888,0.0225345,0.998895,0.0412384,0.982302,-0.0297944,0.18492,\
// 270.728,0,360,0,270.728,202.5,0,0,1,0.988482,-0.0158514,0.150507,0.0181267,0.999741,-0.0137573,-0.15025,0.0163271,0.988513,\
// 266.644,0,360,0,266.644,202.5,0,0,1,-0.148123,0.0470433,0.987849,0.0226372,0.998768,-0.0441689,-0.98871,0.0158198,-0.149005,\
// 265.753,0,360,0,265.753,202.5,0,0,1,-0.987518,0.0180548,-0.156469,0.018992,0.99981,-0.00449679,0.156358,-0.00741229,-0.987673,\
// 268.228",
// "527.008,0,320,0,527.008,180,0,0,1,0.30894,0.0124199,-0.951001,-0.0106465,0.999897,0.00959986,0.951022,0.00715901,0.30904,\
// 539.871,0,320,0,539.871,180,0,0,1,0.94793,-0.0269944,-0.317333,0.0255449,0.999636,-0.00872831,0.317453,0.000167585,0.948274,\
// 547.848,0,320,0,547.848,180,0,0,1,0.949426,0.0149656,0.313634,-0.0238263,0.999417,0.0244377,-0.313085,-0.0306745,0.94923,\
// 552.857,0,320,0,552.857,180,0,0,1,0.297381,0.0106519,0.9547,0.00570208,0.9999,-0.0129323,-0.954742,0.00928961,0.29729,\
// 543.86"
// };

// lj 120fov undistored 640x360
// static std::string camParams[2] = {"245.24,0,320,0,245.24,180,0,0,1,0.929907,-0.00350479,-0.367778,0.0177237,0.99922,0.0352912,0.367367,-0.0393359,0.929244,\
// 232.248,0,320,0,232.248,180,0,0,1,0.392193,0.0103731,0.919825,0.0198202,0.999609,-0.0197238,-0.91967,0.0259667,0.391834,\
// 231.924,0,320,0,231.924,180,0,0,1,-0.926222,0.0339591,0.375447,0.0181173,0.998793,-0.0456457,-0.376544,-0.0354759,-0.925719,\
// 242.916,0,320,0,242.916,180,0,0,1,-0.373838,0.000405399,-0.927494,0.0199929,0.999771,-0.00762135,0.927279,-0.0213924,-0.373761,\
// 237.582",
// "527.008,0,320,0,527.008,180,0,0,1,0.30894,0.0124199,-0.951001,-0.0106465,0.999897,0.00959986,0.951022,0.00715901,0.30904,\
// 539.871,0,320,0,539.871,180,0,0,1,0.94793,-0.0269944,-0.317333,0.0255449,0.999636,-0.00872831,0.317453,0.000167585,0.948274,\
// 547.848,0,320,0,547.848,180,0,0,1,0.949426,0.0149656,0.313634,-0.0238263,0.999417,0.0244377,-0.313085,-0.0306745,0.94923,\
// 552.857,0,320,0,552.857,180,0,0,1,0.297381,0.0106519,0.9547,0.00570208,0.9999,-0.0129323,-0.954742,0.00928961,0.29729,\
// 543.86"
// };

// lj 120fov distored 640x360
// static std::string camParams[2] = {"4707.87,0,320,0,4707.87,180,0,0,1,0.996319,0.0658722,0.0548534,-0.018665,0.791263,-0.611191,-0.0836639,0.607918,0.78958,\
// 4710.84,0,320,0,4710.84,180,0,0,1,0.98047,0.116299,0.158596,0.00588197,0.788715,-0.614731,-0.19658,0.603659,0.772627,\
// 4777.36,0,320,0,4777.36,180,0,0,1,0.981721,-0.104227,-0.159254,-0.0156062,0.789831,-0.613126,0.189688,0.604403,0.773767,\
// 4710.54,0,320,0,4710.54,180,0,0,1,0.995508,-0.0776303,-0.0541957,0.0282772,0.790099,-0.612326,0.0903551,0.608043,0.788745,\
// 4710.69",
// "527.008,0,320,0,527.008,180,0,0,1,0.30894,0.0124199,-0.951001,-0.0106465,0.999897,0.00959986,0.951022,0.00715901,0.30904,\
// 539.871,0,320,0,539.871,180,0,0,1,0.94793,-0.0269944,-0.317333,0.0255449,0.999636,-0.00872831,0.317453,0.000167585,0.948274,\
// 547.848,0,320,0,547.848,180,0,0,1,0.949426,0.0149656,0.313634,-0.0238263,0.999417,0.0244377,-0.313085,-0.0306745,0.94923,\
// 552.857,0,320,0,552.857,180,0,0,1,0.297381,0.0106519,0.9547,0.00570208,0.9999,-0.0129323,-0.954742,0.00928961,0.29729,\
// 543.86"
// };


// sensing 800x360
// static std::string camParams[2] = {"270.692,0,400,0,270.692,225,0,0,1,-0.699073,-0.0444219,-0.713669,-0.010572,0.998602,-0.0518015,0.714972,-0.0286681,-0.698565,\
// 308.62,0,400,0,308.62,225,0,0,1,0.727784,0.00465722,-0.68579,-0.0127062,0.999897,-0.00669394,0.685688,0.0135855,0.727768,\
// 285.141,0,400,0,285.141,225,0,0,1,0.752196,0.0235382,0.658518,-0.00953023,0.999646,-0.0248456,-0.65887,0.0124129,0.752155,\
// 275.802,0,400,0,275.802,225,0,0,1,-0.670237,0.0422871,0.740941,-0.0134661,0.997518,-0.0691117,-0.742025,-0.0562988,-0.668005,\
// 280.472",
// "527.008,0,320,0,527.008,180,0,0,1,0.30894,0.0124199,-0.951001,-0.0106465,0.999897,0.00959986,0.951022,0.00715901,0.30904,\
// 539.871,0,320,0,539.871,180,0,0,1,0.94793,-0.0269944,-0.317333,0.0255449,0.999636,-0.00872831,0.317453,0.000167585,0.948274,\
// 547.848,0,320,0,547.848,180,0,0,1,0.949426,0.0149656,0.313634,-0.0238263,0.999417,0.0244377,-0.313085,-0.0306745,0.94923,\
// 552.857,0,320,0,552.857,180,0,0,1,0.297381,0.0106519,0.9547,0.00570208,0.9999,-0.0129323,-0.954742,0.00928961,0.29729,\
// 543.86"
// };

//lijing undistored 960x540
static std::string camParams[2] = {"506.997,0,480,0,506.997,270,0,0,1,0.840386,0.0154345,-0.541769,8.28463e-10,0.999594,0.0284776,0.541988,-0.0239322,0.840045,\
528.538,0,480,0,528.538,270,0,0,1,0.840453,-0.0112144,0.541769,-1.13672e-07,0.999786,0.0206953,-0.541885,-0.0173935,0.840273,\
517.768",
"368.053,0,480,0,368.053,270,0,0,1,0.747661,0.0236313,-0.66366,4.21091e-09,0.999367,0.0355849,0.664081,-0.0266055,0.747187,\
360.786,0,480,0,360.786,270,0,0,1,0.747555,0.0267741,0.66366,-9.15444e-08,0.999187,-0.0403101,-0.6642,0.030134,0.746947,\
364.419"
};

//lijing undistored 720x405
// static std::string camParams[2] = {"308.136,0,360,0,308.136,202.5,0,0,1,0.786393,0.0049988,-0.617706,-2.46991e-10,0.999967,0.00809226,0.617727,-0.00636369,0.786367,\
// 324.665,0,360,0,324.665,202.5,0,0,1,0.7864,0.00369678,0.617706,-3.96339e-08,0.999982,-0.00598453,-0.617717,0.00470621,0.786386,\
// 316.4",
// "217.492,0,360,0,217.492,202.5,0,0,1,0.668746,0.0405719,-0.742384,-9.88056e-11,0.99851,0.0545694,0.743491,-0.0364931,0.667749,\
// 217.907,0,360,0,217.907,202.5,0,0,1,0.668813,0.0394459,0.742384,-2.13497e-08,0.998591,-0.0530593,-0.743431,0.0354867,0.667871,\
// 217.7"
// };

#else if DISTRIBUTED_STRUCT
//205 structure
// static std::string camParams[2] = {"448.947,0,320,0,448.947,180,0,0,1,0.184235,0.0264681,-0.982526,-0.00994292,0.999636,0.0250646,0.982832,0.00515138,0.184431,\
// 460.455,0,320,0,460.455,180,0,0,1,0.950664,-0.0259506,-0.309136,0.0291518,0.999559,0.00573974,0.308851,-0.0144684,0.951,\
// 473.401,0,320,0,473.401,180,0,0,1,0.948512,0.0235424,0.315866,-0.0292872,0.99948,0.0134523,-0.315385,-0.0220105,0.948708,\
// 494.546,0,320,0,494.546,180,0,0,1,0.21866,0.00304694,0.975796,0.0086784,0.999949,-0.00506705,-0.975762,0.00957631,0.218623,\
// 466.928",
// "527.008,0,320,0,527.008,180,0,0,1,0.30894,0.0124199,-0.951001,-0.0106465,0.999897,0.00959986,0.951022,0.00715901,0.30904,\
// 539.871,0,320,0,539.871,180,0,0,1,0.94793,-0.0269944,-0.317333,0.0255449,0.999636,-0.00872831,0.317453,0.000167585,0.948274,\
// 547.848,0,320,0,547.848,180,0,0,1,0.949426,0.0149656,0.313634,-0.0238263,0.999417,0.0244377,-0.313085,-0.0306745,0.94923,\
// 552.857,0,320,0,552.857,180,0,0,1,0.297381,0.0106519,0.9547,0.00570208,0.9999,-0.0129323,-0.954742,0.00928961,0.29729,\
// 543.86"
// };



#endif

static void Stringsplit(string str, const char split, vector<string>& res)
{
    istringstream iss(str);	// 输入流
    string token;			// 接收缓冲区
    res.clear();
    while (getline(iss, token, split))	// 以split为分隔符
    {
        res.push_back(token);
    }
}

static float radian2degree(float x)
{
    return x*180/M_PI;
}
 
// Calculates rotation matrix to euler angles
// The result is the same as MATLAB except the order
// of the euler angles ( x and z are swapped ).
static Vec3f rotationMatrixToEulerAngles(Mat &m)
{
 
    // assert(isRotationMatrix(R));
     Mat R;
     m.convertTo(R, CV_64FC1);
    float sy = sqrt(R.at<double>(0,0) * R.at<double>(0,0) +  R.at<double>(1,0) * R.at<double>(1,0) );
 
    bool singular = sy < 1e-6; // If
 
    float x, y, z;
    if (!singular)
    {
        x = atan2(R.at<double>(2,1) , R.at<double>(2,2));
        y = atan2(-R.at<double>(2,0), sy);
        z = atan2(R.at<double>(1,0), R.at<double>(0,0));
    }
    else
    {
        x = atan2(-R.at<double>(1,2), R.at<double>(1,1));
        y = atan2(-R.at<double>(2,0), sy);
        z = 0;
    }
    return Vec3f(radian2degree(x), radian2degree(y), radian2degree(z));
}
class ocvStitcher
{
    public:
    ocvStitcher(stStitcherCfg cfg):
    m_imgWidth(cfg.width), m_imgHeight(cfg.height), m_id(cfg.id),num_images(cfg.num_images), m_cfgpath(cfg.cfgPath), 
    match_conf(cfg.matchConf), conf_thresh(cfg.adjusterConf), blend_strength(cfg.blendStrength),
    cameraExThres(cfg.stitchercameraExThres), cameraInThres(cfg.stitchercameraInThres)
    {

        finder = makePtr<SurfFeaturesFinder>();

        seam_work_aspect = min(1.0, sqrt(1e5 / (m_imgHeight*m_imgWidth)));
        // seam_work_aspect = 1;//min(1.0, sqrt(1e5 / (m_imgHeight*m_imgWidth)));

        // camK.reserve(num_images);
        corners.reserve(num_images); 
        blenderMask.reserve(num_images);

        cameraK = Mat(Size(3,3), CV_32FC1);
        for(int i=0;i<num_images;i++)
        {
            cameraR.push_back(Mat(Size(3,3), CV_32FC1));
            camK.push_back(Mat(Size(3,3), CV_32FC1));
        }

        useDefaultCamParams();
// #ifdef GENERATE_SO
//         useDefaultCamParams();
//         presetParaOk = true;
// #else
//         (initCamParams(m_cfgpath) == RET_OK) ? (presetParaOk = true) : (presetParaOk = false);
// #endif
        spdlog::debug("stitcher {} constructor completed!", m_id);
    }

    ~ocvStitcher()
    {

    }

    int verifyCamParams()
    {
        //compare cameras and camK & cameraR
        // for(int i=0;i<4;i++)
        // {
        //     spdlog::debug("stitcher[{}], cameras[{}] R:",m_id, i);
        //     std::cout<<cameras[i].R<<endl;
        // }

        // for(int i=0;i<4;i++)
        // {
        //     spdlog::debug("stitcher[{}], cameras[{}] angles:",m_id, i);
        //     Vec3f angles = rotationMatrixToEulerAngles(cameras[i].R);
        //     std::cout<<angles<<endl;
        // }
        
        // for(int i=0;i<4;i++)
        // {
        //     spdlog::debug("stitcher[{}], cameraR[{}] matrix:",m_id, i);
        //     std::cout<<cameraR[i]<<endl;
        // }

        // for(int i=0;i<4;i++)
        // {
        //     spdlog::debug("stitcher[{}], cameraR[{}] angles:",m_id, i);
        //     Vec3f angles = rotationMatrixToEulerAngles(cameraR[i]);
        //     std::cout<<angles<<endl;
        // }

        for(int i=0;i<num_images;i++)
        {
            Vec3f defaultAngle = rotationMatrixToEulerAngles(cameraR[i]);
            Vec3f estimatedAngle = rotationMatrixToEulerAngles(cameras[i].R);
            double diff = norm(defaultAngle, estimatedAngle);
            spdlog::debug("camera[{}] extrinsic diff:{}", i, diff);
            // std::cout<<"angle:"<<endl<<defaultAngle<<endl<<estimatedAngle<<endl;
            if(diff > cameraExThres)
            {
                spdlog::warn("environment is not suitable for calibration, init failed");
                return RET_ERR;
            }
            
            Vec2f defaultIntrinsic = Vec2f{camK[i].at<float>(0,0), camK[i].at<float>(1,1)};
            Vec2f estimatedIntrinsic = Vec2f{cameras[i].K().at<double>(0,0), cameras[i].K().at<double>(1,1)};
            // cout<<"instrinsic:"<<endl<<defaultIntrinsic<<endl<<estimatedIntrinsic<<endl;
            diff = norm(defaultIntrinsic, estimatedIntrinsic);
            spdlog::debug("camera[{}] intrinsic diff:{}", i, diff);
            if(diff > cameraInThres)
            {
                spdlog::warn("environment is not suitable for calibration, init failed");
                return RET_ERR;
            }
        }

        return RET_OK; 
        
    }

    int useDefaultCamParams()
    {
        vector<string> res;
        Stringsplit(camParams[m_id-1], ',', res);
        spdlog::debug("useDefaultCamParams,res size:{}", res.size());
        for(int i=0;i<num_images;i++)
        {
            for(int mi=0;mi<3;mi++)
            {
                for(int mj=0;mj<3;mj++)
                {
                    camK[i].at<float>(mi,mj) = stof(res[18*i+mi*3+mj]);
                    cameraR[i].at<float>(mi,mj) = stof(res[18*i+9+mi*3+mj]);
                }
            }

            // cout<<K[i]<<endl;
            // cout<<R[i]<<endl;
        }
        warped_image_scale = stof(res.back());

        return RET_OK;
    }

    int initCamParams(std::string cfgpath)
    {
        vector<string> res;
        string filename = cfgpath + "cameraparaout_" + to_string(m_id) + ".txt";
        std::ifstream fin(filename, std::ios::in);
        if(!fin.is_open())
        {
            spdlog::warn("no. {} can not open camerapara file, no preset parameters found, init all!", m_id);
            return RET_ERR;
        }
        std::string str;
        int linenum = 0;
        int paranum = 0;

        while(getline(fin,str))
        {
            linenum++;
            if (std::string::npos != str.find(":"))
            {
                paranum++;
            }
        }

        fin.clear();
        fin.seekg(0, ios::beg);

        spdlog::debug("linenum:{}, paranum:{}", linenum, paranum);
        // cout<<"linenum:"<<linenum<<",paranum:"<<paranum<<endl;
        if(linenum == 0)
        {
            spdlog::info("no. {} no preset parameters found, init all!", m_id);
            return RET_ERR;
        }

        for(int i=0;i<6*(paranum-1);i++)
            getline(fin,str);

        fin >> str;
        // cout << "params time:" << str << endl;
        spdlog::debug("params timestamp:{}", str);

        for(int i=0;i<num_images;i++)
        {
            fin >> str;
            Stringsplit(str, ',', res);
            if(res.size() != 18)
            {
                spdlog::warn("camera {} preset parameter incorrect, init all!", m_id);
                return RET_ERR;
            }

            for(int mi=0;mi<3;mi++)
            {
                for(int mj=0;mj<3;mj++)
                {
                    camK[i].at<float>(mi,mj) = stof(res[mi*3+mj]);
                    cameraR[i].at<float>(mi,mj) = stof(res[9+mi*3+mj]);
                }
            }

            // cout<<camK[i]<<endl;
            // cout<<cameraR[i]<<endl;

        }
        fin >> str;
        warped_image_scale = stof(str);

        return RET_OK;
    }

    int saveCameraParams()
    {
        std::time_t tt = chrono::system_clock::to_time_t (chrono::system_clock::now());
        struct std::tm * ptm = std::localtime(&tt);
        // std::cout << "Now (local time): " << std::put_time(ptm,"%F-%H-%M-%S") << '\n';

        string filename = m_cfgpath + "cameraparaout_" + to_string(m_id) + ".txt";
        ofstream fout(filename, std::ofstream::out | std::ofstream::app);

        if(!fout.is_open())
        {
            spdlog::warn("no. {} can not open camerapara file, save camera para failed!", m_id);
            return RET_ERR;
        }

        fout << std::put_time(ptm,"%F-%H-%M-%S:\n");
        
        for(int idx=0;idx<num_images;idx++)
        {
            for(int mi=0;mi<3;mi++)
            {
                for(int mj=0;mj<3;mj++)
                {
                    fout << camK[idx].at<float>(mi,mj) << ",";
                    // cameraK.at<float>(mi,mj) = cameras[idx].K().at<double>(mi,mj);
                }
            }  
            for(int mi=0;mi<3;mi++)
            {
                for(int mj=0;mj<3;mj++)
                {
                    fout << cameraR[idx].at<float>(mi,mj) << ",";
                    // cameraR[idx].at<float>(mi,mj) = cameraR[idx].at<float>(mi,mj);
                }
            }
            fout << "\n";
        }
        fout << warped_image_scale << "\n";

        return RET_OK;
    }

    // init mode, 1:initall, 2:use default, init seam, 3:read cfg, init seam
    int init(vector<Mat> &imgs, int initMode)
    {
        // if(initMode || !presetParaOk)
        //     return initAll(imgs);
        // else
        //     return initSeam(imgs);

        if(enInitALL == initMode)
            return initAll(imgs);
        else if(enInitByDefault)
        {
            useDefaultCamParams();
            return initSeam(imgs);
        }
        else if(enInitByCfg)
        {
            if(RET_OK ==initCamParams(m_cfgpath))
                return initSeam(imgs);
            else
                return initAll(imgs);
        }
        else
        {
            spdlog::critical("invalid init mode, exit");
            return RET_ERR;
        }


    }

    int initAll(vector<Mat> &imgs)
    {
        spdlog::debug("***********stitcher {} init start**************", m_id);
        auto t = getTickCount();
        auto app_start_time = getTickCount();
        vector<ImageFeatures> features(num_images);
        vector<Mat> seamSizedImgs = vector<Mat>(num_images);
        for(int i=0;i<num_images;i++)
        {
            // std::vector<cv::Rect> rois = {cv::Rect(m_imgWidth/2, 0, m_imgWidth/2, m_imgHeight)};
            // if(i == num_images - 1)
            //     (*finder)(imgs[i], features[i], rois);
            // else
                (*finder)(imgs[i], features[i]);

            features[i].img_idx = i;
            spdlog::debug("Features in image {} : {}", i, features[i].keypoints.size());

            resize(imgs[i], seamSizedImgs[i], Size(), seam_work_aspect, seam_work_aspect, INTER_LINEAR_EXACT);
        }

        finder->collectGarbage();

        vector<MatchesInfo> pairwise_matches;
        matcher = makePtr<BestOf2NearestMatcher>(false, match_conf);

        (*matcher)(features, pairwise_matches);
        matcher->collectGarbage();

        // std::string save_graph_to = "11match.txt";
        // ofstream f(save_graph_to.c_str());
        // std::vector<String> img_names;
        // img_names.push_back("5.png");
        // img_names.push_back("6.png");
        // img_names.push_back("7.png");
        // img_names.push_back("8.png");
        // f << matchesGraphAsString(img_names, pairwise_matches, conf_thresh);

        estimator = makePtr<HomographyBasedEstimator>();

        if (!(*estimator)(features, pairwise_matches, cameras))
        {
            spdlog::critical("Homography estimation failed.");
            return -1;
        }

        // LOGLN("***********after estimator**************" << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");
        spdlog::debug("***********after estimator**************, {} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);

        for (size_t i = 0; i < cameras.size(); ++i)
        {
            Mat R;
            cameras[i].R.convertTo(R, CV_32F);
            cameras[i].R = R;
            // LOGLN("Initial camera intrinsics #" << i << ":\nK:\n" << cameras[i].K() << "\nR:\n" << cameras[i].R);
        }

        adjuster = makePtr<detail::BundleAdjusterRay>();
        adjuster->setConfThresh(conf_thresh);
        Mat_<uchar> refine_mask = Mat::zeros(3, 3, CV_8U);

        refine_mask(0,0) = 1;
        refine_mask(0,1) = 1;
        refine_mask(0,2) = 1;
        refine_mask(1,1) = 1;
        refine_mask(1,2) = 1;
        adjuster->setRefinementMask(refine_mask);
        if (!(*adjuster)(features, pairwise_matches, cameras))
        {
            spdlog::critical("Camera parameters adjusting failed.");
            return -1;
        }

        //  for (size_t i = 0; i < cameras.size(); ++i)
        // {
        //     LOGLN("Camera #" << i+1 << ":\nK:\n" << cameras[i].K() << "\nR:\n" << cameras[i].R);
        // }

        // Find median focal length

        spdlog::debug("***********after Camera parameters adjusting Find median focal length**************");

        vector<double> focals;

        for (size_t i = 0; i < cameras.size(); ++i)
        {
            // LOGLN("Camera #" << i+1 << ":\nK:\n" << cameras[i].K() << "\nR:\n" << cameras[i].R);
            focals.push_back(cameras[i].focal);
            // LOGLN("focal:"<<cameras[i].focal);
        }

        sort(focals.begin(), focals.end());
        float estimated_warped_image_scale;
        // float warped_image_scale;
        if (focals.size() % 2 == 1)
            estimated_warped_image_scale = static_cast<float>(focals[focals.size() / 2]);
        else
            estimated_warped_image_scale = static_cast<float>(focals[focals.size() / 2 - 1] + focals[focals.size() / 2]) * 0.5f;
        
        // warped_image_scale = static_cast<float>(cameras[2].focal);

        /* takes about 0.1ms*/
        t = getTickCount();
        vector<Mat> rmats;
        for (size_t i = 0; i < cameras.size(); ++i)
            rmats.push_back(cameras[i].R.clone());
        waveCorrect(rmats, detail::WAVE_CORRECT_HORIZ);
        for (size_t i = 0; i < cameras.size(); ++i)
            cameras[i].R = rmats[i];

        // LOGLN("***********waveCorrect takes::**************" << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");
        spdlog::debug("***********waveCorrect takes::************** {} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);

        spdlog::debug("Warping images (auxiliary)... ");

        /*every camera use same intrinsic parameter, make no big difference*/
        
        // for (size_t i = 0; i < cameras.size(); ++i)
        // {
        //     // LOGLN("Initial camera intrinsics #" << i << ":\nK:\n" << cameras[i].K());
        //     // cameras[i] = cameras[0];
        //     // cameras[i].R = rmats[i];
        //     LOGLN("camera R:" << i << "\n" << cameras[i].R);
        //     LOGLN("after set Initial camera intrinsics #" << i+1 << ":\nK:\n" << cameras[i].K());
        //     LOGLN("default camera intrinsics #" << i+1 << ":\nK:\n" << camK[i]);
        //     // fout << "camera " << i << ":\n";
        // }

        // verify camera parameters
       if(RET_OK == verifyCamParams())
       {
           for(int i=0;i<num_images;i++)
           {
            cameraR[i] = cameras[i].R.clone();
            camK[i] = cameras[i].K().clone();
            warped_image_scale = estimated_warped_image_scale;
           }
       }
#ifdef DEV_MODE
        /*save camera parsms*/
        saveCameraParams();
#endif

        vector<UMat> masks(num_images);
        // Preapre images masks
        for (int i = 0; i < num_images; ++i)
        {
            masks[i].create(seamSizedImgs[i].size(), CV_8U);
            masks[i].setTo(Scalar::all(255));
        }

        spdlog::debug("warped_image_scale:{},seam_work_aspect:{}", warped_image_scale, seam_work_aspect);
        // warper_creator = makePtr<cv::CylindricalWarperGpu>();
        warper_creator = makePtr<cv::SphericalWarperGpu>();
        seamfinder_warper = warper_creator->create(static_cast<float>(warped_image_scale * seam_work_aspect));

        vector<UMat> images_warped(num_images);
        // vector<Size> sizes(num_images);
        vector<UMat> masks_warped(num_images);

        for (int i = 0; i < num_images; ++i)
        {
            // Mat_<float> K;
            camK[i].convertTo(camK[i], CV_32F);

            // LOGLN("cameras[i] for sephere warp #" << i << "\n" << cameras[i].K());
            // LOGLN("cam K for sephere warp #" << i << "\n" << camK[i]);
            // LOGLN("cam R for sephere warp #" << i << "\n" << cameras[i].R);

            auto K = camK[i].clone();

            float swa = (float)seam_work_aspect;
            K(0,0) *= swa; K(0,2) *= swa;
            K(1,1) *= swa; K(1,2) *= swa;

            corners[i] = seamfinder_warper->warp(seamSizedImgs[i], K, cameraR[i], INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            // corners[i] = warper->warp(seamSizedImgs[i], camK[i], cameras[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            sizes[i] = images_warped[i].size();

            seamfinder_warper->warp(masks[i], K, cameraR[i], INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);
#ifdef DEV_MODE
            imwrite(std::to_string(i)+"-images_warped.png", images_warped[i]);
            imwrite(std::to_string(i)+"-masks_warped.png", masks_warped[i]);
#endif
            // LOGLN("****first warp:" << i << ":\n corners  " << corners[i] << "\n size:" << sizes[i]);
        }

        for(int i=0;i<num_images;i++)
        {
            // blenderMask[i] = masks_warped[i].getMat(ACCESS_RW);
            masks_warped[i].copyTo(blenderMask[i]);
            // blenderMask[i].setTo(Scalar::all(255));
        }

        t = getTickCount();
        vector<UMat> images_warped_f(num_images);
        for (int i = 0; i < num_images; ++i)
            images_warped[i].convertTo(images_warped_f[i], CV_32F);
        
        // compensator = ExposureCompensator::createDefault(ExposureCompensator::GAIN_BLOCKS);

        // compensator->feed(corners, images_warped, masks_warped);
        seam_finder = makePtr<detail::GraphCutSeamFinder>(GraphCutSeamFinderBase::COST_COLOR);
        // seam_finder = makePtr<detail::NoSeamFinder>();
        seam_finder->find(images_warped_f, corners, masks_warped);

        // for (int i = 0; i < num_images; ++i)
        // {
        //     LOGLN("corners:" << i << ":\n" << corners[i]);
        //     imwrite(std::to_string(i)+"-seamfinder-masks_warped.png", masks_warped[i]);
        // }

        spdlog::debug("***********seam_finder takes:::************** {} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);

        // Release unused memory
        seamSizedImgs.clear();
        images_warped.clear();
        images_warped_f.clear();
        masks.clear();

        spdlog::debug("*************Compositing...");

        // seam_work_aspect is 1, use the same warper
        blender_warper = warper_creator->create(warped_image_scale);
        for (int i = 0; i < num_images; ++i)
        {
            // LOGLN("Update corners and sizes camK i #" << i << ":\nK:\n" << camK[i]);
            Rect roi = blender_warper->warpRoi(Size(m_imgWidth,m_imgHeight), camK[i], cameraR[i]);
            // Rect roi = warper->warpRoi(sz, K, cameras[i].R);
            corners[i] = roi.tl();
            sizes[i] = roi.size();

            // LOGLN("****:" << i << ":\n corners  " << corners[i] << "\n size:" << sizes[i]);
        }

        Ptr<Blender> blender;
        float blend_width;
        Mat img, img_warped, img_warped_s;
        Mat dilated_mask, seam_mask;
        Mat mask, maskwarped;

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            spdlog::debug("Compositing image {}", img_idx);

            // Read image and resize it if necessary
            img = imgs[img_idx];
            Size img_size = img.size();

            // Warp the current image
            blender_warper->warp(img, camK[img_idx], cameraR[img_idx], INTER_LINEAR, BORDER_REFLECT, img_warped);

            // // Warp the current image mask
            mask.create(img_size, CV_8U);
            mask.setTo(Scalar::all(255));
            blender_warper->warp(mask, camK[img_idx], cameraR[img_idx], INTER_NEAREST, BORDER_CONSTANT, compensatorMaskWarped[img_idx]);

            // Compensate exposure
            // compensator->apply(img_idx, corners[img_idx], img_warped, maskwarped);

            // imwrite("maskwarped.png", mask_warped);

            img_warped.convertTo(img_warped_s, CV_16S);
            img_warped.release();
            img.release();
            // mask.release();

            dilate(masks_warped[img_idx], dilated_mask, Mat());
            // imwrite(std::to_string(img_idx)+"-dilated_mask.png", dilated_mask);
            resize(dilated_mask, seam_mask, compensatorMaskWarped[img_idx].size(), 0, 0, INTER_LINEAR_EXACT);
            // mask_warped = seam_mask & mask_warped;
            blenderMask[img_idx] = seam_mask & compensatorMaskWarped[img_idx];

            // imwrite("blendmask.png", blenderMask[img_idx]);
            // imwrite(std::to_string(img_idx)+"-blenderMask.png", blenderMask[img_idx]);
            // imwrite(std::to_string(img_idx)+"-img_warped.png", img_warped);

            if (!blender)
            {
                blender = Blender::createDefault(Blender::MULTI_BAND, true);
                dst_sz = resultRoi(corners, sizes).size();
                blend_width = sqrt(static_cast<float>(dst_sz.area())) * blend_strength / 100.f;
                spdlog::trace("blend_width: {}, dst_sz: [{},{}]", blend_width, dst_sz.width, dst_sz.height);
                if (blend_width < 1.f)
                    blender = Blender::createDefault(Blender::NO, true);
                else
                {
                    MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(blender.get());
                    mb->setNumBands(static_cast<int>(ceil(log(blend_width)/log(2.)) - 1.));
                    spdlog::trace("Multi-band blender, number of bands: {}", mb->numBands());
                }
                blender->prepare(corners, sizes);
            }

            // Blend the current image
            blender->feed(img_warped_s, blenderMask[img_idx], corners[img_idx]);
        }

        Mat result, result_mask;
        blender->blend(result, result_mask);

        spdlog::debug("init ok takes : {} ms", ((getTickCount() - app_start_time) / getTickFrequency()) * 1000);
#ifdef DEV_MODE
        imwrite(to_string(m_id)+"_ocv.png", result);
#endif

        return 0;
    }
    int initSeam(vector<Mat> &imgs)
    {
        spdlog::debug("***********init seam start**************");
        auto t = getTickCount();
        auto app_start_time = getTickCount();
        // vector<ImageFeatures> features(num_images);
        vector<Mat> seamSizedImgs = vector<Mat>(num_images);
        for(int i=0;i<num_images;i++)
        {
            // (*finder)(imgs[i], features[i]);
            // features[i].img_idx = i;
            // LOGLN("Features in image #" << i+1 << ": " << features[i].keypoints.size());

            resize(imgs[i], seamSizedImgs[i], Size(), seam_work_aspect, seam_work_aspect, INTER_LINEAR_EXACT);
        }

        vector<UMat> masks(num_images);
        // Preapre images masks
        for (int i = 0; i < num_images; ++i)
        {
            masks[i].create(seamSizedImgs[i].size(), CV_8U);
            masks[i].setTo(Scalar::all(255));
        }

        // warper_creator = makePtr<cv::CylindricalWarperGpu>();
        warper_creator = makePtr<cv::SphericalWarperGpu>();
        seamfinder_warper = warper_creator->create(static_cast<float>(warped_image_scale * seam_work_aspect));

        vector<UMat> images_warped(num_images);
        vector<UMat> masks_warped(num_images);

        for (int i = 0; i < num_images; ++i)
        {
            auto K = camK[i].clone();
            float swa = (float)seam_work_aspect;
            K(0,0) *= swa; K(0,2) *= swa;
            K(1,1) *= swa; K(1,2) *= swa;

            corners[i] = seamfinder_warper->warp(seamSizedImgs[i], K, cameraR[i], INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            // corners[i] = warper->warp(seamSizedImgs[i], camK[i], cameras[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            sizes[i] = images_warped[i].size();

            seamfinder_warper->warp(masks[i], K, cameraR[i], INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);

            // imwrite(std::to_string(i)+"-images_warped.png", images_warped[i]);
            // imwrite(std::to_string(i)+"-masks_warped.png", masks_warped[i]);

            // LOGLN("****first warp:" << i << ":\n corners  " << corners[i] << "\n size:" << sizes[i]);
            // spdlog::warn("images_warped channels::{}", images_warped[i].channels());
        }

        t = getTickCount();
        vector<UMat> images_warped_f(num_images);
        for (int i = 0; i < num_images; ++i)
            images_warped[i].convertTo(images_warped_f[i], CV_32F);
        
        compensator = ExposureCompensator::createDefault(ExposureCompensator::GAIN_BLOCKS);
        compensator->feed(corners, images_warped, masks_warped);
        seam_finder = makePtr<detail::GraphCutSeamFinder>(GraphCutSeamFinderBase::COST_COLOR);
        // seam_finder = makePtr<detail::NoSeamFinder>();
        seam_finder->find(images_warped_f, corners, masks_warped);

        // for (int i = 0; i < num_images; ++i)
        // {
        //     LOGLN("corners:" << i << ":\n" << corners[i]);
        //     imwrite(std::to_string(i)+"-seamfinder-masks_warped.png", masks_warped[i]);
        // }

        spdlog::debug("***********seam_finder takes:::************** {} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);

        // Release unused memory
        seamSizedImgs.clear();
        images_warped.clear();
        images_warped_f.clear();
        masks.clear();

        spdlog::debug("*************Compositing*************.");

        // seam_work_aspect is 1, use the same warper
        blender_warper = warper_creator->create(warped_image_scale);
        for (int i = 0; i < num_images; ++i)
        {
            Rect roi = blender_warper->warpRoi(Size(m_imgWidth,m_imgHeight), camK[i], cameraR[i]);
            // Rect roi = warper->warpRoi(sz, K, cameras[i].R);
            corners[i] = roi.tl();
            sizes[i] = roi.size();

            // LOGLN("****:" << i << ":\n corners  " << corners[i] << "\n size:" << sizes[i]);
        }

        Ptr<Blender> blender;
        float blend_width;
        Mat img, img_warped, img_warped_s;
        Mat dilated_mask, seam_mask;
        Mat mask;

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            spdlog::debug("Compositing image {}", img_idx);

            // Read image and resize it if necessary
            img = imgs[img_idx];
            Size img_size = img.size();

            // Warp the current image
            blender_warper->warp(img, camK[img_idx], cameraR[img_idx], INTER_LINEAR, BORDER_REFLECT, img_warped);

            // // Warp the current image mask
            mask.create(img_size, CV_8U);
            mask.setTo(Scalar::all(255));
            blender_warper->warp(mask, camK[img_idx], cameraR[img_idx], INTER_NEAREST, BORDER_CONSTANT, compensatorMaskWarped[img_idx]);

            // Compensate exposure
            // compensator->apply(img_idx, corners[img_idx], img_warped, compensatorMaskWarped[img_idx]);

            // imwrite("maskwarped.png", mask_warped);

            img_warped.convertTo(img_warped_s, CV_16S);
            img_warped.release();
            img.release();
            // mask.release();

            dilate(masks_warped[img_idx], dilated_mask, Mat());
            // imwrite(std::to_string(img_idx)+"-dilated_mask.png", dilated_mask);
            resize(dilated_mask, seam_mask, compensatorMaskWarped[img_idx].size(), 0, 0, INTER_LINEAR_EXACT);
            // mask_warped = seam_mask & mask_warped;
            blenderMask[img_idx] = seam_mask & compensatorMaskWarped[img_idx];

            // imwrite("blendmask.png", blenderMask[img_idx]);
            // imwrite(std::to_string(img_idx)+"-blenderMask.png", blenderMask[img_idx]);
            // imwrite(std::to_string(img_idx)+"-img_warped.png", img_warped);

            if (!blender)
            {
                blender = Blender::createDefault(Blender::MULTI_BAND, true);
                dst_sz = resultRoi(corners, sizes).size();
                blend_width = sqrt(static_cast<float>(dst_sz.area())) * blend_strength / 100.f;
                spdlog::trace("blend_width: {}, dst_sz: [{},{}]", blend_width, dst_sz.width, dst_sz.height);
                if (blend_width < 1.f)
                    blender = Blender::createDefault(Blender::NO, true);
                else
                {
                    MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(blender.get());
                    mb->setNumBands(static_cast<int>(ceil(log(blend_width)/log(2.)) - 1.));
                    spdlog::trace("Multi-band blender, number of bands: {}", mb->numBands());
                }
                blender->prepare(corners, sizes);
            }

            // Blend the current image
            blender->feed(img_warped_s, blenderMask[img_idx], corners[img_idx]);
        }

        Mat result, result_mask;
        blender->blend(result, result_mask);

        spdlog::debug("init ok takes : {} ms", ((getTickCount() - app_start_time) / getTickFrequency()) * 1000);
#ifdef DEV_MODE
        imwrite(to_string(m_id)+"_ocv.png", result);
#endif

        return 0;
    }

    void process(vector<Mat> &imgs, Mat &ret)
    {
        spdlog::debug("stitcher {} process start", m_id);
        auto app_start_time = getTickCount();

        Ptr<Blender> blender;
        float blend_width;
        Mat img, img_warped, img_warped_s;

        static int cnt = 0;

        if(cnt++ == 200)
        {
            stmtx.lock();
            updateMask(imgs);
            stmtx.unlock();
            cnt = 0;
            spdlog::debug("***********updateMask***********");
        }

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            // LOGLN("Compositing image #" << img_idx+1);
            auto t = getTickCount();
            // Read image and resize it if necessary
            img = imgs[img_idx];
            Size img_size = img.size();

            // Warp the current image
            stmtx.lock();
            blender_warper->warp(img, camK[img_idx], cameraR[img_idx], INTER_LINEAR, BORDER_REFLECT, img_warped);
            stmtx.unlock();
            // imwrite(std::to_string(img_idx)+"-img_warped.png", img_warped);

            // LOGLN("process:::"<<"id:"<<m_id<< "  index::"<< img_idx<<"  K:"<<cameraK<<"cameraR[img_idx]:  "<< cameraR[img_idx]);

            // Compensate exposure
            // compensator->apply(img_idx, corners[img_idx], img_warped, compensatorMaskWarped[img_idx]);

            img_warped.convertTo(img_warped_s, CV_16S);
            img_warped.release();
            img.release();

            if (!blender)
            {
                blender = Blender::createDefault(Blender::MULTI_BAND, true);
                // Size dst_sz = resultRoi(corners, sizes).size();
                blend_width = sqrt(static_cast<float>(dst_sz.area())) * blend_strength / 100.f;
                spdlog::trace("blend_width: {}, dst_sz: [{},{}]", blend_width, dst_sz.width, dst_sz.height);
                if (blend_width < 1.f)
                    blender = Blender::createDefault(Blender::NO, true);
                else
                {
                    MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(blender.get());
                    mb->setNumBands(static_cast<int>(ceil(log(blend_width)/log(2.)) - 1.));
                    spdlog::trace("Multi-band blender, number of bands: {}", mb->numBands());
                }
                blender->prepare(corners, sizes);
            }
            // LOGLN("before feed takes : " << ((getTickCount() - t) / getTickFrequency()) * 1000 << " ms");
            // Blend the current image
            blender->feed(img_warped_s, blenderMask[img_idx], corners[img_idx]);
            // LOGLN("****:" << "id:"<<m_id<<"  idx:"<< img_idx << ":\n corners  " << corners[img_idx] << "\n size:" << sizes[img_idx]);
        }

        Mat result, result_mask;
        blender->blend(result, result_mask);
        result.convertTo(ret, CV_8U);

        spdlog::debug("stitcher {} process takes : {:03.3f} ms", m_id, ((getTickCount() - app_start_time) / getTickFrequency()) * 1000);
        // imwrite("ocvprocess.png", ret);

        
    }

    void updateMask(vector<Mat> &imgs)
    {
        auto t = getTickCount();
        vector<UMat> images_warped(num_images);
        vector<UMat> masks_warped(num_images);
        vector<UMat> images_warped_f(num_images);
        vector<UMat> masks(num_images);
        vector<Point> seamfinder_corners = vector<Point>(num_images);
        vector<Mat> seamSizedImgs = vector<Mat>(num_images);

        for (int i = 0; i < num_images; ++i)
        {
            resize(imgs[i], seamSizedImgs[i], Size(), seam_work_aspect, seam_work_aspect, INTER_LINEAR_EXACT);

            auto K = camK[i].clone();
            float swa = (float)seam_work_aspect;
            K(0,0) *= swa; K(0,2) *= swa;
            K(1,1) *= swa; K(1,2) *= swa;

            seamfinder_corners[i] = seamfinder_warper->warp(seamSizedImgs[i], K, cameraR[i], INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
            images_warped[i].convertTo(images_warped_f[i], CV_32F);
            masks[i].create(seamSizedImgs[i].size(), CV_8U);
            masks[i].setTo(Scalar::all(255));
            seamfinder_warper->warp(masks[i], K, cameraR[i], INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);
        }

        seam_finder->find(images_warped_f, seamfinder_corners, masks_warped);

        Mat dilated_mask, seam_mask;
        Mat mask;

        for (int img_idx = 0; img_idx < num_images; ++img_idx)
        {
            mask.create(imgs[img_idx].size(), CV_8U);
            mask.setTo(Scalar::all(255));
            blender_warper->warp(mask, camK[img_idx], cameraR[img_idx], INTER_NEAREST, BORDER_CONSTANT, compensatorMaskWarped[img_idx]);

            dilate(masks_warped[img_idx], dilated_mask, Mat());
            resize(dilated_mask, seam_mask, compensatorMaskWarped[img_idx].size(), 0, 0, INTER_LINEAR_EXACT);
            blenderMask[img_idx] = seam_mask & compensatorMaskWarped[img_idx];
        }

        spdlog::debug("updateMask takes : {:03.2f} ms", ((getTickCount() - t) / getTickFrequency()) * 1000);
    }


    public:
    short int num_images;
    Ptr<FeaturesFinder> finder;
    Ptr<FeaturesMatcher> matcher;
    Ptr<Estimator> estimator;
    vector<CameraParams> cameras;
    Ptr<WarperCreator> warper_creator;
    Ptr<detail::BundleAdjusterBase> adjuster;
    Ptr<ExposureCompensator> compensator;
    Ptr<SeamFinder> seam_finder;
    Ptr<RotationWarper> blender_warper, seamfinder_warper;
    vector<Mat_<float>> camK;// = vector<Mat_<float>>(num_images);
    vector<Point> corners = vector<Point>(num_images);
    vector<Mat> blenderMask = vector<Mat>(num_images);
    vector<Mat> compensatorMaskWarped = vector<Mat>(num_images);
    // vector<Mat> seamSizedImgs = vector<Mat>(num_images);

    vector<Mat> cameraR;
    Mat cameraK;
    int m_imgWidth, m_imgHeight;
    double seam_work_aspect;
    Size dst_sz;
    vector<Size> sizes = vector<Size>(num_images);

    float warped_image_scale;
    short int m_id;

    bool presetParaOk;
    std::string m_cfgpath;

    float match_conf, conf_thresh, blend_strength, cameraExThres, cameraInThres;

    // Ptr<Blender> blender;

};

#endif