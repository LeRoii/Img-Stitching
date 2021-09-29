/***********************************
 * this is demo for camera
 * email:   cuixiaolei@tztek.com 
 * date:    20190925
 * ********************************/
#ifndef _GMSLCAM_H_
#define _GMSLCAM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h> /* getopt_long() */
#include <fcntl.h> /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <unistd.h>

#include <sched.h>

#include <cuda_runtime.h>
#include "yuyv2rgb.cuh"
#include <opencv2/opencv.hpp>
#include <thread>
#include <string>

#define CAMERA_NUM 7

cv::Mat intrinsic_matrix[3];
cv::Mat distortion_coeffs[3];

int cutpara[4][4] = {
	{36,53,888,440},
	{27,47,898,438},
	{45,60,882,421},
	{45,60,882,421}
};


class gmslCamera;

static int index_get = 0;
static int index_pro = 0;

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define BUFFER_LENGHT           4

int PIXL_WIDTH = 1920;
int PIXL_HEIGHT	= 1080;



char *image_yuyv_cuda_ = NULL;
char *image_rgb_cuda_ = NULL;


unsigned char *show_buf;

int pixel_w = PIXL_WIDTH, pixel_h = PIXL_HEIGHT;
int screen_w = 1280, screen_h = 720;


struct buffer {
        void *start;
        size_t length;
};


struct buffer *buffers;
unsigned char *yuyv_buf;
static int fd_video = -1;

char dev_name[24] = "/dev/video0";

int width = PIXL_WIDTH;
int height = PIXL_HEIGHT;
static int v4l2_format = V4L2_PIX_FMT_YUYV;

static int flag_enable_trigger = 0;


static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
        int r;
        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);
        return r;
}

const unsigned char uchar_clipping_table[] = {
	0, 0, 0, 0, 0, 0, 0, 0, // -128 - -121
	0, 0, 0, 0, 0, 0, 0, 0, // -120 - -113
	0, 0, 0, 0, 0, 0, 0, 0, // -112 - -105
	0, 0, 0, 0, 0, 0, 0, 0, // -104 -  -97
	0, 0, 0, 0, 0, 0, 0, 0, //  -96 -  -89
	0, 0, 0, 0, 0, 0, 0, 0, //  -88 -  -81
	0, 0, 0, 0, 0, 0, 0, 0, //  -80 -  -73
	0, 0, 0, 0, 0, 0, 0, 0, //  -72 -  -65
	0, 0, 0, 0, 0, 0, 0, 0, //  -64 -  -57
	0, 0, 0, 0, 0, 0, 0, 0, //  -56 -  -49
	0, 0, 0, 0, 0, 0, 0, 0, //  -48 -  -41
	0, 0, 0, 0, 0, 0, 0, 0, //  -40 -  -33
	0, 0, 0, 0, 0, 0, 0, 0, //  -32 -  -25
	0, 0, 0, 0, 0, 0, 0, 0, //  -24 -  -17
	0, 0, 0, 0, 0, 0, 0, 0, //  -16 -   -9
	0, 0, 0, 0, 0, 0, 0, 0, //   -8 -   -1
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
	31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
	60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
	89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
	114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
	137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182,
	183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205,
	206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228,
	229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251,
	252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 256-263
	255, 255, 255, 255, 255, 255, 255, 255, // 264-271
	255, 255, 255, 255, 255, 255, 255, 255, // 272-279
	255, 255, 255, 255, 255, 255, 255, 255, // 280-287
	255, 255, 255, 255, 255, 255, 255, 255, // 288-295
	255, 255, 255, 255, 255, 255, 255, 255, // 296-303
	255, 255, 255, 255, 255, 255, 255, 255, // 304-311
	255, 255, 255, 255, 255, 255, 255, 255, // 312-319
	255, 255, 255, 255, 255, 255, 255, 255, // 320-327
	255, 255, 255, 255, 255, 255, 255, 255, // 328-335
	255, 255, 255, 255, 255, 255, 255, 255, // 336-343
	255, 255, 255, 255, 255, 255, 255, 255, // 344-351
	255, 255, 255, 255, 255, 255, 255, 255, // 352-359
	255, 255, 255, 255, 255, 255, 255, 255, // 360-367
	255, 255, 255, 255, 255, 255, 255, 255, // 368-375
	255, 255, 255, 255, 255, 255, 255, 255, // 376-383
};
const int clipping_table_offset = 128;


/** Clip a value to the range 0<val<255. For speed this is done using an
 * array, so can only cope with numbers in the range -128<val<383.
 */
static unsigned char CLIPVALUE(int val)
{
	// Old method (if)
	/* val = val < 0 ? 0 : val; */
	/* return val > 255 ? 255 : val; */
	
	// New method (array)
	return uchar_clipping_table[val + clipping_table_offset];
}

/**
 * Conversion from YUV to RGB.
 * The normal conversion matrix is due to Julien (surname unknown):
 *
 * [ R ]   [  1.0   0.0     1.403 ] [ Y ]
 * [ G ] = [  1.0  -0.344  -0.714 ] [ U ]
 * [ B ]   [  1.0   1.770   0.0   ] [ V ]
 *
 * and the firewire one is similar:
 *
 * [ R ]   [  1.0   0.0     0.700 ] [ Y ]
 * [ G ] = [  1.0  -0.198  -0.291 ] [ U ]
 * [ B ]   [  1.0   1.015   0.0   ] [ V ]
 *
 * Corrected by BJT (coriander's transforms RGB->YUV and YUV->RGB
 *                   do not get you back to the same RGB!)
 * [ R ]   [  1.0   0.0     1.136 ] [ Y ]
 * [ G ] = [  1.0  -0.396  -0.578 ] [ U ]
 * [ B ]   [  1.0   2.041   0.002 ] [ V ]
 *
 */
static void YUV2RGB(const unsigned char y, const unsigned char u, const unsigned char v, unsigned char* r,
                    unsigned char* g, unsigned char* b)
{
	const int y2 = (int)y;
	const int u2 = (int)u - 128;
	const int v2 = (int)v - 128;
	//std::cerr << "YUV=("<<y2<<","<<u2<<","<<v2<<")"<<std::endl;
	
	// This is the normal YUV conversion, but
	// appears to be incorrect for the firewire cameras
	/* int r2 = y2 + ( (v2*91947) >> 16); */
	/* int g2 = y2 - ( ((u2*22544) + (v2*46793)) >> 16 ); */
	/* int b2 = y2 + ( (u2*115999) >> 16); */

	// This is an adjusted version (UV spread out a bit)
	int r2 = y2 + ((v2 * 37221) >> 15);
	int g2 = y2 - (((u2 * 12975) + (v2 * 18949)) >> 15);
	int b2 = y2 + ((u2 * 66883) >> 15);
	
	// Cap the values.
	*r = CLIPVALUE(r2);
	*g = CLIPVALUE(g2);
	*b = CLIPVALUE(b2);
}

static void yuyv2rgb(char* YUV, char* RGB, int NumPixels)
{
	int i, j;
	unsigned char y0, y1, u, v;
	unsigned char r, g, b;
	
	for (i = 0, j = 0; i < (NumPixels << 1); i += 4, j += 6)
	{
		y0 = (unsigned char)YUV[i + 0];
		u = (unsigned char)YUV[i + 1];
		y1 = (unsigned char)YUV[i + 2];
		v = (unsigned char)YUV[i + 3];
		YUV2RGB(y0, u, v, &r, &g, &b);
		RGB[j + 0] = r;
		RGB[j + 1] = g;
		RGB[j + 2] = b;
		YUV2RGB(y1, u, v, &r, &g, &b);
		RGB[j + 3] = r;
		RGB[j + 4] = g;
		RGB[j + 5] = b;
	}
}


static void init_cuda(void)
{
	// Number of CUDA devices
	int devCount = 0;
	cudaGetDeviceCount(&devCount);
	printf("CUDA Device Query...\n");
	printf("There are %d CUDA devices.\n", devCount);

	if (0 == devCount)
		errno_exit("No cuda device found\n");
/*
	// Iterate through devices
	for (int i = 0; i < devCount; ++i) {
		// Get device properties
		printf("\nCUDA Device #%d\n", i);
		cudaDeviceProp devProp;
		cudaGetDeviceProperties(&devProp, i);
		printDevProp(devProp);
	}
*/
}

static void process_image(const void* src, int size)
{
	struct timeval ts;

	int yuv_size = size * sizeof(char);

	gettimeofday(&ts, NULL);
        printf("[%lu.%lu]\tbefore yuyv2rgb computation\n", ts.tv_sec, ts.tv_usec);
	yuyv2rgb((char *)src, (char *)show_buf, yuv_size / 3);
	gettimeofday(&ts, NULL);
        printf("[%lu.%lu]\tyuyv2rgb computation done\n", ts.tv_sec, ts.tv_usec);
        printf("[%lu.%lu]\tprocess image index = %d\n", ts.tv_sec, ts.tv_usec, ++index_pro);
}

static void stop_capturing(gmslCamera &cam);
static void open_device(gmslCamera &cam);
static void init_device(gmslCamera &cam);
static void uninit_device(gmslCamera &cam);
static void start_capturing(gmslCamera &cam);
static void stop_capturing(gmslCamera &cam);
static void close_device(gmslCamera &cam);
static void process_image_cuda(const void *src, int size, gmslCamera &cam);

struct stCamCfg
{
    int width;
    int height;
	int distoredWidth;
	int distoredHeight;
	int retWidth;
	int retHeight;
	int id;
    char name[20];
};

bool m_save = false;
std::string command;

void keyBoardListener()
{
	while(1)
	{
		std::cin>>command;
		std::cout<<"cmd:"<<command<<std::endl;
		if(command == "s")
		{
			m_save = true;
			command = "";
		}
	}
}

class gmslCamera
{
public:
    gmslCamera(stCamCfg &camcfg):m_imgWidth(camcfg.width),m_imgHeight(camcfg.height),
	m_retWidth(camcfg.retWidth),m_retHeight(camcfg.retHeight),
	m_distoredWidth(camcfg.distoredWidth),m_distoredHeight(camcfg.distoredHeight), m_id(camcfg.id)
    {
        strcpy(m_dev_name, camcfg.name);
        init();

		std::thread thsave(keyBoardListener);
		thsave.detach();

		intrinsic_matrix[0] = (cv::Mat_<double>(3,3) << 853.417882746302, 0, 483.001902270090,
                        0, 959.666714085956, 280.450178308760,
                        0, 0, 1);

		distortion_coeffs[0] = (cv::Mat_<double>(1,4) << -0.368584528301156, 0.0602436114872144, 0, 0);

		intrinsic_matrix[1] = (cv::Mat_<double>(3,3) << 904.312885512814, 0, 465.749959799123,
                        0, 1015.99397200986, 262.536941689523,
                        0, 0, 1);

		distortion_coeffs[1] = (cv::Mat_<double>(1,4) << -0.400286349184253, 0.0804443894863993, 0, 0);

		intrinsic_matrix[2] = (cv::Mat_<double>(3,3) << 888.663951715643, 0, 497.720535634466,
                        0, 999.827428536261, 269.990239946566,
                        0, 0, 1);

		distortion_coeffs[2] = (cv::Mat_<double>(1,4) << -0.368432689228974, 0.00515158537987999, 0, 0);

    }
    ~gmslCamera()
    {
        stop_capturing(*this);
        uninit_device(*this);

        free(show_buf);
        free(yuyv_buf);
            
        close_device(*this);
    }
    void init()
    {
        // int width_l = 3840 , height_l = 2160;
        // width_l = str2int(argv[2]);
        // height_l = str2int(argv[3]);
    //	printf("int argv[2]=%d\r\n", width_l);
    //	printf("int argv[3]=%d\r\n", height_l);

        PIXL_WIDTH = m_imgWidth;
        PIXL_HEIGHT = m_imgHeight;
        
        width = PIXL_WIDTH;
        height = PIXL_HEIGHT;
        pixel_w = PIXL_WIDTH;
        pixel_h = PIXL_HEIGHT;

        // strcpy(dev_name, "/dev/video0");
    //	printf("dev_name= %s\r\n", dev_name);
        m_show_buf = (unsigned char *)malloc(PIXL_WIDTH*PIXL_HEIGHT*3);
        memset(m_show_buf, 0, PIXL_WIDTH*PIXL_HEIGHT*3);
        yuyv_buf = (unsigned char *)malloc(PIXL_WIDTH*PIXL_HEIGHT*3);
        memset(yuyv_buf, 0, PIXL_WIDTH*PIXL_HEIGHT*3);
        //CUDA init
        init_cuda();
    
        open_device(*this);
        init_device(*this);
        start_capturing(*this);
        
		m_camimg = cv::Mat(m_imgHeight, m_imgWidth, CV_8UC3);
        m_ret = cv::Mat(m_retHeight, m_retWidth, CV_8UC3);
        m_camimg.data = m_show_buf;

        //Allocate cuda memory for device
        int rgb_image_size = width * height * 3;
        int yuyv_image_size = width * height * 2;
        cudaError_t ret = cudaMalloc((void **)&m_image_yuyv_cuda, yuyv_image_size * sizeof(char));
        if (cudaSuccess != ret) {
            printf("Fail to allocate cuda memory %d\n", ret);
            exit(EXIT_FAILURE);
        }
        ret = cudaMemset((void *)m_image_yuyv_cuda, 0, yuyv_image_size * sizeof(char));
        if (cudaSuccess != ret) {
            printf("Fail to set cuda memory1 %d\n", ret);
            exit(EXIT_FAILURE);
        }
        ret = cudaMalloc((void **)&m_image_rgb_cuda, rgb_image_size * sizeof(char));
        if (cudaSuccess != ret) {
            printf("Fail to allocate cuda memory %d\n", ret);
            exit(EXIT_FAILURE);
        }
        ret = cudaMemset((void *)m_image_rgb_cuda, 0, rgb_image_size * sizeof(char));
        if (cudaSuccess != ret) {
            printf("Fail to set cuda2 memory %d\n", ret);
            exit(EXIT_FAILURE);
        }
    }

    void read_frame()
    {
		std::string filename = std::to_string(m_id)+"-";
		int matrixIdx = m_id-1;
		int *rectPara = cutpara[matrixIdx];

        struct timeval ts;
        struct v4l2_buffer buf;
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

            // get picture
        gettimeofday(&ts, NULL);
        printf("\n[%lu.%lu]\tbefore get picture m_fd_video:%d\n", ts.tv_sec, ts.tv_usec, m_fd_video);
        if (-1 == xioctl(m_fd_video, VIDIOC_DQBUF, &buf)) 
                errno_exit("VIDIOC_DQBUF");
        gettimeofday(&ts, NULL);
        printf("[%lu.%lu]\tget image index = %d.\n", ts.tv_sec, ts.tv_usec, ++index_get);
        printf("[%lu.%lu]\tget image is ok.\n", ts.tv_sec, ts.tv_usec);
        
        // deal with image data
        //process_image(buffers[buf.index].start, buf.bytesused);
        process_image_cuda(m_buffers[buf.index].start, buf.bytesused, *this);
        gettimeofday(&ts, NULL);
        printf("[%lu.%lu]\tprocess image is ok\n", ts.tv_sec, ts.tv_usec);

        // cv::Mat showimg;
        cv::resize(m_camimg, m_ret, cv::Size(m_retWidth, m_retHeight));
        cv::cvtColor(m_ret, m_ret, cv::COLOR_BGR2RGB);

        //undistored*********
        cv::imwrite(filename+"dist.png", m_ret);
        // cv::Size image_size = m_ret.size();
        // cv::Size undistorSize = image_size;
        // cv::Mat mapx = cv::Mat(undistorSize,CV_32FC1);
        // cv::Mat mapy = cv::Mat(undistorSize,CV_32FC1);
        // cv::Mat R = cv::Mat::eye(3,3,CV_32F);
        // cv::Mat optMatrix = getOptimalNewCameraMatrix(intrinsic_matrix[matrixIdx], distortion_coeffs[matrixIdx], image_size, 1, undistorSize, 0);
        // cv::initUndistortRectifyMap(intrinsic_matrix[matrixIdx],distortion_coeffs[matrixIdx], R, optMatrix, undistorSize, CV_32FC1, mapx, mapy);
        // // cv::Mat t = cv::Mat(undistorSize,CV_8UC3);
        // cv::remap(m_ret,m_ret,mapx, mapy, cv::INTER_CUBIC);
        // m_ret = m_ret(cv::Rect(rectPara[0],rectPara[1],rectPara[2],rectPara[3]));
        // cv::resize(m_ret, m_ret, cv::Size(m_retWidth, m_retHeight));
        // cv::imwrite(filename+"undist.png", m_ret);
        //undistored end
		
		
        printf("cap frame ok\n");
        if(m_save)
		{
			static int cnt = 0;
			cv::imwrite(filename+std::to_string(cnt++)+".png", m_ret);
			m_save = false;
		}
        // cv::imshow(m_dev_name, showimg);
        // cv::waitKey(33);
        
        if (-1 == xioctl(m_fd_video, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF"); 
    }

    public:
    int m_imgWidth, m_imgHeight;
	int m_retWidth, m_retHeight;
	int m_distoredWidth, m_distoredHeight;
    char *m_image_yuyv_cuda;
    char *m_image_rgb_cuda;
    unsigned char *m_show_buf;
    int m_fd_video;
    buffer *m_buffers;
    char m_dev_name[30];
    cv::Mat m_camimg, m_ret;
	int m_id;
    
};

static void process_image_cuda(const void *src, int size, gmslCamera &cam)
{
	struct timeval ts;

	int yuv_size = size * sizeof(char);

	gettimeofday(&ts, NULL);
        printf("[%lu.%lu]\tbefore copy image_data(CPU to GPU)\n", ts.tv_sec, ts.tv_usec);
	cudaError_t ret = cudaMemcpy(cam.m_image_yuyv_cuda, src, yuv_size, cudaMemcpyHostToDevice);
	gettimeofday(&ts, NULL);
        printf("[%lu.%lu]\tcopy image_data(CPU to GPU) done\n", ts.tv_sec, ts.tv_usec);

	if (cudaSuccess != ret) {
		printf("cudaMemcpy fail %d\n", ret);
	}
	const int block_size = 256;
	const int num_blocks = yuv_size / (4*block_size);


	gettimeofday(&ts, NULL);
        printf("[%lu.%lu]\tbefore yuyv2rgb computation\n", ts.tv_sec, ts.tv_usec);
	yuyv2rgb_cuda(cam.m_image_yuyv_cuda, cam.m_image_rgb_cuda, num_blocks, block_size);
	gettimeofday(&ts, NULL);
        printf("[%lu.%lu]\tyuyv2rgb computation done\n", ts.tv_sec, ts.tv_usec);


	int rgb_size = size / 2 * 3 * sizeof(char);

	gettimeofday(&ts, NULL);
        printf("[%lu.%lu]\tbefore copy image_data(GPU to CPU)\n", ts.tv_sec, ts.tv_usec);
	ret = cudaMemcpy(cam.m_show_buf, cam.m_image_rgb_cuda, rgb_size, cudaMemcpyDeviceToHost);
	gettimeofday(&ts, NULL);
        printf("[%lu.%lu]\tcopy image_data(GPU to CPU) done\n", ts.tv_sec, ts.tv_usec);
        printf("[%lu.%lu]\tcuda process image index = %d\n", ts.tv_sec, ts.tv_usec, ++index_pro);

	if (cudaSuccess != ret) {
		printf("cudaMemcpy fail %d\n", ret);
	}
}

static void stop_capturing(gmslCamera &cam)
{
        printf("%s\n", __func__);
        enum v4l2_buf_type type;

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(cam.m_fd_video, VIDIOC_STREAMOFF, &type))
            errno_exit("VIDIOC_STREAMOFF");

}

static void start_capturing(gmslCamera &cam)
{
        unsigned int i;
        enum v4l2_buf_type type;

        struct v4l2_control ctrl_mode;

        printf("%s\n", __func__);

        for (i = 0; i < BUFFER_LENGHT; ++i) {
                struct v4l2_buffer buf;

                CLEAR(buf);
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

				//enqueue an empty (capturing) or filled (output) buffer in the driver’s incoming queue
                if (-1 == xioctl(cam.m_fd_video, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
        }

        //set streaming on
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (-1 == xioctl(cam.m_fd_video, VIDIOC_STREAMON, &type))
                errno_exit("VIDIOC_STREAMON");
}

static void uninit_device(gmslCamera &cam)
{
        unsigned int i;
        printf("%s\n", __func__);

        for (i = 0; i < BUFFER_LENGHT; ++i)
                if (-1 == munmap(cam.m_buffers[i].start, buffers[i].length))
                        errno_exit("munmap");
        free(cam.m_buffers);
}



static void init_mmap(gmslCamera &cam)
{
        int n_buffers;
        struct v4l2_requestbuffers req;
        printf("%s\n", __func__);
        CLEAR(req);

        req.count = BUFFER_LENGHT;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

		//开启内存映射或用户指针I/O
        if (-1 == xioctl(cam.m_fd_video, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "memory mapping\n", cam.m_dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf(stderr, "Insufficient buffer memory on %s\n",
                         cam.m_dev_name);
                exit(EXIT_FAILURE);
        }

        cam.m_buffers = (struct buffer*)calloc(req.count, sizeof(*cam.m_buffers));

        if (!cam.m_buffers) {
                fprintf(stderr, "Out of memory\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = n_buffers;

				//Query the status of a buffer
                if (-1 == xioctl(cam.m_fd_video, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                cam.m_buffers[n_buffers].length = buf.length;
                cam.m_buffers[n_buffers].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              cam.m_fd_video, buf.m.offset);

                if (MAP_FAILED == cam.m_buffers[n_buffers].start)
                        errno_exit("mmap");
        }
}


static void init_device(gmslCamera &cam)
{
        struct v4l2_capability cap;	 //视频设备的功能，对应命令VIDIOC_QUERYCAP 
        struct v4l2_format fmt;	//帧的格式，对应命令VIDIOC_G_FMT、VIDIOC_S_FMT等
        printf("%s\n", __func__);
        if (-1 == xioctl(cam.m_fd_video, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s is no V4L2 device\n",
                                 cam.m_dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is no video capture device\n",
                         cam.m_dev_name);
                exit(EXIT_FAILURE);
        }

        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                fprintf(stderr, "%s does not support streaming i/o\n",
                    cam.m_dev_name);
                exit(EXIT_FAILURE);
        }

        CLEAR(fmt);
		
		
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = width;
        fmt.fmt.pix.height = height;
        fmt.fmt.pix.pixelformat = v4l2_format;
        fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

        if (-1 == xioctl(cam.m_fd_video, VIDIOC_S_FMT, &fmt))
                errno_exit("VIDIOC_S_FMT");

	usleep(10000);



        init_mmap(cam);
}

static void close_device(gmslCamera &cam)
{       
        printf("%s\n", __func__);
        if (-1 == close(cam.m_fd_video))
                errno_exit("close fd_video");

        cam.m_fd_video = -1;
}

static void open_device(gmslCamera &cam)
{
        printf("%s\n", __func__);
        cam.m_fd_video = open(cam.m_dev_name, O_RDWR /* required */ /*| O_NONBLOCK*/, 0);



        if (-1 == cam.m_fd_video) {
                fprintf(stderr, "Cannot open '%s': %d, %s\n",
                         cam.m_dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

}

int str2int(const char* str)
{
    int temp = 0;
    const char* p = str;
    if(str == NULL) return 0;
    if(*str == '-' || *str == '+')
    {
        str ++;
    }
    while( *str != 0)
    {
        if( *str < '0' || *str > '9')
        {
            break;
        }
        temp = temp*10 +(*str -'0');
        str ++;
    }
    if(*p == '-')
    {
        temp = -temp;
    }
    return temp;
}


#endif