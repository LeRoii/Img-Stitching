/*
 * @Author: your name
 * @Date: 2021-11-10 11:06:27
 * @LastEditTime: 2021-11-10 14:41:00
 * @LastEditors: your name
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /0929IS/include/nvcam.hpp
 */
/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <opencv2/core/cuda.hpp>
#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>

#include <queue>
#include <mutex>
#include <condition_variable>

#include "NvEglRenderer.h"
#include "NvUtils.h"
#include "NvCudaProc.h"
#include "nvbuf_utils.h"

#include "camera_v4l2-cuda.h"
#include "spdlog/spdlog.h"
#include "stitcherconfig.h"
#include "helper_timer.h"


static bool quit = false;

std::vector<std::vector<int>> rectPara(2);// = {65,105,1788,886};
cv::Mat intrinsic_matrix[2];
cv::Mat distortion_coeffs[2];

using namespace std;

std::mutex m_mtx[8];
std::condition_variable con[8];
std::mutex changeszmtx;

static void
set_defaults(camcontext_t * ctx)
{
    memset(ctx, 0, sizeof(camcontext_t));

    ctx->cam_devname = "/dev/video0";
    ctx->cam_fd = -1;
    ctx->cam_pixfmt = V4L2_PIX_FMT_YUYV;
    ctx->cam_w = 640;
    ctx->cam_h = 480;
    ctx->frame = 0;
    ctx->save_n_frame = 0;

    ctx->g_buff = NULL;
    ctx->capture_dmabuf = true;
    ctx->renderer = NULL;
    ctx->fps = 30;

    ctx->enable_cuda = false;
    ctx->egl_image = NULL;
    ctx->egl_display = EGL_NO_DISPLAY;

    ctx->enable_verbose = false;
}

static nv_color_fmt nvcolor_fmt[] =
{
    /* TODO: add more pixel format mapping */
    {V4L2_PIX_FMT_UYVY, NvBufferColorFormat_UYVY},
    {V4L2_PIX_FMT_VYUY, NvBufferColorFormat_VYUY},
    {V4L2_PIX_FMT_YUYV, NvBufferColorFormat_YUYV},
    {V4L2_PIX_FMT_YVYU, NvBufferColorFormat_YVYU},
    {V4L2_PIX_FMT_GREY, NvBufferColorFormat_GRAY8},
    {V4L2_PIX_FMT_YUV420M, NvBufferColorFormat_YUV420},
};

static NvBufferColorFormat
get_nvbuff_color_fmt(unsigned int v4l2_pixfmt)
{
    unsigned i;

    for (i = 0; i < sizeof(nvcolor_fmt) / sizeof(nvcolor_fmt[0]); i++)
    {
        if (v4l2_pixfmt == nvcolor_fmt[i].v4l2_pixfmt)
            return nvcolor_fmt[i].nvbuff_color;
    }

    return NvBufferColorFormat_Invalid;
}

static bool
nvbuff_do_clearchroma (int dmabuf_fd)
{
  NvBufferParams params = {0};
  void *sBaseAddr[3] = {NULL};
  int ret = 0;
  int size;
  unsigned i;

  ret = NvBufferGetParams (dmabuf_fd, &params);
  if (ret != 0)
    ERROR_RETURN("%s: NvBufferGetParams Failed \n", __func__);

  for (i = 1; i < params.num_planes; i++) {
    ret = NvBufferMemMap (dmabuf_fd, i, NvBufferMem_Read_Write, &sBaseAddr[i]);
    if (ret != 0)
      ERROR_RETURN("%s: NvBufferMemMap Failed \n", __func__);

    /* Sync device cache for CPU access since data is from VIC */
    ret = NvBufferMemSyncForCpu (dmabuf_fd, i, &sBaseAddr[i]);
    if (ret != 0)
      ERROR_RETURN("%s: NvBufferMemSyncForCpu Failed \n", __func__);

    size = params.height[i] * params.pitch[i];
    memset (sBaseAddr[i], 0x80, size);

    /* Sync CPU cache for VIC access since data is from CPU */
    ret = NvBufferMemSyncForDevice (dmabuf_fd, i, &sBaseAddr[i]);
    if (ret != 0)
      ERROR_RETURN("%s: NvBufferMemSyncForDevice Failed \n", __func__);

    ret = NvBufferMemUnMap (dmabuf_fd, i, &sBaseAddr[i]);
    if (ret != 0)
      ERROR_RETURN("%s: NvBufferMemUnMap Failed \n", __func__);
  }

  return true;
}

// static bool
// display_initialize(camcontext_t * ctx)
// {
//     /* Create EGL renderer */
//     ctx->renderer = NvEglRenderer::createEglRenderer("renderer0",
//             ctx->cam_w/2, ctx->cam_h/2, 0, 0);
//     if (!ctx->renderer)
//         ERROR_RETURN("Failed to create EGL renderer");
//     ctx->renderer->setFPS(ctx->fps);

//     if (ctx->enable_cuda)
//     {
//         /* Get defalut EGL display */
//         ctx->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
//         if (ctx->egl_display == EGL_NO_DISPLAY)
//             ERROR_RETURN("Failed to get EGL display connection");

//         /* Init EGL display connection */
//         if (!eglInitialize(ctx->egl_display, NULL, NULL))
//             ERROR_RETURN("Failed to initialize EGL display connection");
//     }

//     return true;
// }

static bool
camera_initialize(camcontext_t * ctx)
{
    struct v4l2_format fmt;

    /* Open camera device */
    ctx->cam_fd = open(ctx->dev_name, O_RDWR);
    if (ctx->cam_fd == -1)
        ERROR_RETURN("Failed to open camera device %s: %s (%d)",
                ctx->dev_name, strerror(errno), errno);

    /* Set camera output format */
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = ctx->cam_w;
    fmt.fmt.pix.height = ctx->cam_h;
    fmt.fmt.pix.pixelformat = ctx->cam_pixfmt;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (ioctl(ctx->cam_fd, VIDIOC_S_FMT, &fmt) < 0)
        ERROR_RETURN("Failed to set camera output format: %s (%d)",
                strerror(errno), errno);

    /* Get the real format in case the desired is not supported */
    memset(&fmt, 0, sizeof fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(ctx->cam_fd, VIDIOC_G_FMT, &fmt) < 0)
        ERROR_RETURN("Failed to get camera output format: %s (%d)",
                strerror(errno), errno);
    if (fmt.fmt.pix.width != ctx->cam_w ||
            fmt.fmt.pix.height != ctx->cam_h ||
            fmt.fmt.pix.pixelformat != ctx->cam_pixfmt)
    {
        WARN("The desired format is not supported");
        ctx->cam_w = fmt.fmt.pix.width;
        ctx->cam_h = fmt.fmt.pix.height;
        ctx->cam_pixfmt =fmt.fmt.pix.pixelformat;
    }

    // printf("fmt.fmt.pix.pixelformat!!!!!!!!!!!:%d\n", fmt.fmt.pix.pixelformat);

    struct v4l2_streamparm streamparm;
    memset (&streamparm, 0x00, sizeof (struct v4l2_streamparm));
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl (ctx->cam_fd, VIDIOC_G_PARM, &streamparm);

    INFO("Camera ouput format: (%d x %d)  stride: %d, imagesize: %d, frate: %u / %u",
            fmt.fmt.pix.width,
            fmt.fmt.pix.height,
            fmt.fmt.pix.bytesperline,
            fmt.fmt.pix.sizeimage,
            streamparm.parm.capture.timeperframe.denominator,
            streamparm.parm.capture.timeperframe.numerator);

    return true;
}

static bool
init_components(camcontext_t * ctx)
{
    if (!camera_initialize(ctx))
        ERROR_RETURN("Failed to initialize camera device");

    // if (!display_initialize(ctx))
    //     ERROR_RETURN("Failed to initialize display");

    INFO("Initialize v4l2 components successfully");
    return true;
}

static bool
request_camera_buff(camcontext_t *ctx)
{
    /* Request camera v4l2 buffer */
    struct v4l2_requestbuffers rb;
    memset(&rb, 0, sizeof(rb));
    rb.count = V4L2_BUFFERS_NUM;    //缓冲区内缓冲帧的数目，缓冲区中保留4帧
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_DMABUF;
    if (ioctl(ctx->cam_fd, VIDIOC_REQBUFS, &rb) < 0)
        ERROR_RETURN("Failed to request v4l2 buffers: %s (%d)",
                strerror(errno), errno);
    if (rb.count != V4L2_BUFFERS_NUM)
        ERROR_RETURN("V4l2 buffer number is not as desired");

    for (unsigned int index = 0; index < V4L2_BUFFERS_NUM; index++)
    {
        struct v4l2_buffer buf;

        /* Query camera v4l2 buf length */
        memset(&buf, 0, sizeof buf);
        buf.index = index;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_DMABUF;

        if (ioctl(ctx->cam_fd, VIDIOC_QUERYBUF, &buf) < 0)
            ERROR_RETURN("Failed to query buff: %s (%d)",
                    strerror(errno), errno);

        /* TODO: add support for multi-planer
           Enqueue empty v4l2 buff into camera capture plane */
        buf.m.fd = (unsigned long)ctx->g_buff[index].dmabuff_fd;
        if (buf.length != ctx->g_buff[index].size)
        {
            WARN("Camera v4l2 buf length is not expected");
            ctx->g_buff[index].size = buf.length;
        }

        if (ioctl(ctx->cam_fd, VIDIOC_QBUF, &buf) < 0)
            ERROR_RETURN("Failed to enqueue buffers: %s (%d)",
                    strerror(errno), errno);
    }

    return true;
}

static bool
request_camera_buff_mmap(camcontext_t *ctx)
{
    /* Request camera v4l2 buffer */
    struct v4l2_requestbuffers rb;
    memset(&rb, 0, sizeof(rb));
    rb.count = V4L2_BUFFERS_NUM;
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_MMAP;
    if (ioctl(ctx->cam_fd, VIDIOC_REQBUFS, &rb) < 0)
        ERROR_RETURN("Failed to request v4l2 buffers: %s (%d)",
                strerror(errno), errno);
    if (rb.count != V4L2_BUFFERS_NUM)
        ERROR_RETURN("V4l2 buffer number is not as desired");

    for (unsigned int index = 0; index < V4L2_BUFFERS_NUM; index++)
    {
        struct v4l2_buffer buf;

        /* Query camera v4l2 buf length */
        memset(&buf, 0, sizeof buf);
        buf.index = index;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(ctx->cam_fd, VIDIOC_QUERYBUF, &buf) < 0)
            ERROR_RETURN("Failed to query buff: %s (%d)",
                    strerror(errno), errno);

        ctx->g_buff[index].size = buf.length;
        ctx->g_buff[index].start = (unsigned char *)
            mmap (NULL /* start anywhere */,
                    buf.length,
                    PROT_READ | PROT_WRITE /* required */,
                    MAP_SHARED /* recommended */,
                    ctx->cam_fd, buf.m.offset);
        if (MAP_FAILED == ctx->g_buff[index].start)
            ERROR_RETURN("Failed to map buffers");

        if (ioctl(ctx->cam_fd, VIDIOC_QBUF, &buf) < 0)
            ERROR_RETURN("Failed to enqueue buffers: %s (%d)",
                    strerror(errno), errno);
    }

    return true;
}

static bool
prepare_buffers(camcontext_t * ctx)
{
    NvBufferCreateParams input_params = {0};

    /* Allocate global buffer context */
    ctx->g_buff = (nv_buffer *)malloc(V4L2_BUFFERS_NUM * sizeof(nv_buffer));
    if (ctx->g_buff == NULL)
        ERROR_RETURN("Failed to allocate global buffer context");

    input_params.payloadType = NvBufferPayload_SurfArray;
    input_params.width = ctx->cam_w;
    input_params.height = ctx->cam_h;
    input_params.layout = NvBufferLayout_Pitch;

    /* Create buffer and provide it with camera */
    for (unsigned int index = 0; index < V4L2_BUFFERS_NUM; index++)
    {
        int fd;
        NvBufferParams params = {0};

        input_params.colorFormat = get_nvbuff_color_fmt(ctx->cam_pixfmt);
        input_params.nvbuf_tag = NvBufferTag_CAMERA;
        if (-1 == NvBufferCreateEx(&fd, &input_params))
            ERROR_RETURN("Failed to create NvBuffer");

        ctx->g_buff[index].dmabuff_fd = fd;

        if (-1 == NvBufferGetParams(fd, &params))
            ERROR_RETURN("Failed to get NvBuffer parameters");

        /* TODO: add multi-planar support
           Currently only supports YUV422 interlaced single-planar */
        if (ctx->capture_dmabuf) {
            if (-1 == NvBufferMemMap(ctx->g_buff[index].dmabuff_fd, 0, NvBufferMem_Read_Write,
                        (void**)&ctx->g_buff[index].start))
                ERROR_RETURN("Failed to map buffer");
        }
    }

    input_params.colorFormat = get_nvbuff_color_fmt(V4L2_PIX_FMT_YUV420M);
    input_params.colorFormat = NvBufferColorFormat_ABGR32 ;
    input_params.nvbuf_tag = NvBufferTag_NONE;
    /* Create Render buffer */
    // if (-1 == NvBufferCreateEx(&ctx->render_dmabuf_fd, &input_params))
    //     ERROR_RETURN("Failed to create NvBuffer");

    if (ctx->capture_dmabuf) {
        if (!request_camera_buff(ctx))
            ERROR_RETURN("Failed to set up camera buff");
    } else {
        if (!request_camera_buff_mmap(ctx))
            ERROR_RETURN("Failed to set up camera buff");
    }

    INFO("Succeed in preparing stream buffers");
    return true;
}

static bool
start_stream(camcontext_t * ctx)
{
    enum v4l2_buf_type type;

    /* Start v4l2 streaming */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(ctx->cam_fd, VIDIOC_STREAMON, &type) < 0)
        ERROR_RETURN("Failed to start streaming: %s (%d)",
                strerror(errno), errno);

    usleep(200);

    INFO("Camera video streaming on ...");
    return true;
}

static void
signal_handle(int signum)
{
    printf("Quit due to exit command from user!\n");
    quit = true;
}

static bool
stop_stream(camcontext_t * ctx)
{
    enum v4l2_buf_type type;

    /* Stop v4l2 streaming */
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(ctx->cam_fd, VIDIOC_STREAMOFF, &type))
        ERROR_RETURN("Failed to stop streaming: %s (%d)",
                strerror(errno), errno);

    INFO("Camera video streaming off ...");
    return true;
}

class nvCam
{
public:
    nvCam(stCamCfg &camcfg):m_camSrcWidth(camcfg.camSrcWidth),m_camSrcHeight(camcfg.camSrcHeight),
	m_retWidth(camcfg.retWidth),m_retHeight(camcfg.retHeight),
	m_distoredWidth(camcfg.distoredWidth),m_distoredHeight(camcfg.distoredHeight), 
	m_undistoredWidth(camcfg.undistoredWidth),m_undistoredHeight(camcfg.undistoredHeight), 
    m_id(camcfg.id),m_undistor(camcfg.undistor)
    
    {
        if(m_camSrcWidth == 3840)
        {
            //4k camera 1080 undistored
            intrinsic_matrix[0] = (cv::Mat_<double>(3,3) << 1.767104822915813e+03, 0 , 9.674122717568121e+02, 
                                    0, 1.980908029523902e+03, 5.694739251420406e+02,
                                    0, 0, 1);

            distortion_coeffs[0] = (cv::Mat_<double>(1,4) << -0.4066, 0.1044, 0, 0);
            rectPara[0] = vector<int>{65,105,1788,886};

            //4k camera 540 undistored
            intrinsic_matrix[1] = (cv::Mat_<double>(3,3) << 853.417882746302, 0, 483.001902270090,
                                0, 959.666714085956, 280.450178308760,
                                0, 0, 1);

            distortion_coeffs[1] = (cv::Mat_<double>(1,4) << -0.368584528301156, 0.0602436114872144, 0, 0);
            rectPara[1] = vector<int>{36,53,888,440};
        }
        else if(m_camSrcWidth == 1920)
        {
            //imx390 960x540 undistored
            intrinsic_matrix[1] = (cv::Mat_<double>(3,3) << 1.015264419405688e+03, 0, 5.175898502304585e+02,
                                0, 1.011960767907845e+03, 2.927908447845667e+02,
                                0, 0, 1);

            distortion_coeffs[1] = (cv::Mat_<double>(1,4) << -0.6027, 0.2956, 0, 0);
            rectPara[1] = vector<int>{45,64,882,423};

            //imx390 1920x1080 undistored
            intrinsic_matrix[0] = (cv::Mat_<double>(3,3) << 1.946119547414241e+03, 0, 1.016749758038493e+03,
                                0, 1.943374997244887e+03, 5.679760696574299e+02,
                                0, 0, 1);

            distortion_coeffs[0] = (cv::Mat_<double>(1,4) << -0.5554, 0.2303, 0, 0);
            rectPara[0] = vector<int>{95,130,1751,840};
        }
        else
        {
            spdlog::critical("invalid camSrcWidth:{}", m_camSrcWidth);
        }

        set_defaults(&ctx);
        strcpy(ctx.dev_name, camcfg.name);
        ctx.cam_pixfmt = V4L2_PIX_FMT_YUYV;
        ctx.enable_cuda = true;
        ctx.enable_verbose = true;
        ctx.cam_w = m_camSrcWidth;
        ctx.cam_h = m_camSrcHeight;

        bool ok;
        ok = init_components(&ctx);
        ok &= prepare_buffers(&ctx);
        ok &= start_stream(&ctx);

        if(!ok)
            // printf("ERROR: %s(): (line:%d)\n", __FUNCTION__, __LINE__);
            spdlog::critical("ERROR: {}(): (line:{})", __FUNCTION__, __LINE__);

        // NvBufferCreateParams bufparams = {0};
        // retNvbuf = (nv_buffer *)malloc(2*sizeof(nv_buffer));
        // // retNvbuf = (nv_buffer *)malloc(sizeof(nv_buffer));
        // bufparams.payloadType = NvBufferPayload_SurfArray;
        // bufparams.width = 1920;
        // bufparams.height = 1080;
        // bufparams.layout = NvBufferLayout_Pitch;
        // bufparams.colorFormat = NvBufferColorFormat_ARGB32;
        // bufparams.nvbuf_tag = NvBufferTag_NONE;
        // if (-1 == NvBufferCreateEx(&retNvbuf[0].dmabuff_fd, &bufparams))
        //         spdlog::critical("Failed to create NvBuffer 1920");

        // bufparams.width = 960;
        // bufparams.height = 540;
        // if (-1 == NvBufferCreateEx(&retNvbuf[1].dmabuff_fd, &bufparams))
        //         spdlog::critical("Failed to create NvBuffer 960");

        NvBufferCreateParams bufparams = {0};
        retNvbuf = (nv_buffer *)malloc(sizeof(nv_buffer));
        // retNvbuf = (nv_buffer *)malloc(sizeof(nv_buffer));
        bufparams.payloadType = NvBufferPayload_SurfArray;
        bufparams.width = m_distoredWidth;
        bufparams.height = m_distoredHeight;
        bufparams.layout = NvBufferLayout_Pitch;
        bufparams.colorFormat = NvBufferColorFormat_ARGB32;
        bufparams.nvbuf_tag = NvBufferTag_NONE;
        if (-1 == NvBufferCreateEx(&retNvbuf->dmabuff_fd, &bufparams))
                spdlog::critical("Failed to create NvBuffer 1920");

        m_argb = cv::Mat(m_distoredHeight, m_distoredWidth, CV_8UC4);
        m_gpuargb = cv::cuda::GpuMat(m_distoredHeight, m_distoredWidth, CV_8UC4);

        bufparams.width = m_retWidth;
        bufparams.height = m_retHeight;
        if (-1 == NvBufferCreateEx(&ctx.render_dmabuf_fd, &bufparams))
                spdlog::critical("Failed to create NvBuffer ctx->render_dmabuf_fd");

        // m_argb = cv::Mat(m_distoredHeight, m_distoredWidth, CV_8UC4);
        // m_argb[0] = cv::Mat(1080, 1920, CV_8UC4);
        // m_argb[1] = cv::Mat(540, 960, CV_8UC4);

        // m_gpuargb[0] = cv::cuda::GpuMat(1080, 1920, CV_8UC4);
        // m_gpuargb[1] = cv::cuda::GpuMat(540, 960, CV_8UC4);

        m_gpuDistoredImg = cv::cuda::GpuMat(m_undistoredHeight, m_undistoredWidth, CV_8UC4);
        m_gpuUndistoredImg = cv::cuda::GpuMat(m_undistoredHeight, m_undistoredWidth, CV_8UC4);
        m_gpuret = cv::cuda::GpuMat(m_retHeight, m_retWidth, CV_8UC4);

        /* Init the NvBufferTransformParams */
        memset(&transParams, 0, sizeof(transParams));
        transParams.transform_flag = NVBUFFER_TRANSFORM_FILTER;
        transParams.transform_filter = NvBufferTransform_Filter_Smart;

        // m_queue.resize(10);

        cv::Size image_size = cv::Size(1920, 1080);
        cv::Size undistorSize = image_size;

        // cv::Mat mapx[2], mapy[2];
        mapx[0] = cv::Mat(undistorSize,CV_32FC1);
        mapy[0] = cv::Mat(undistorSize,CV_32FC1);
        cv::Mat R = cv::Mat::eye(3,3,CV_32F);
        cv::Mat optMatrix = getOptimalNewCameraMatrix(intrinsic_matrix[0], distortion_coeffs[0], image_size, 1, undistorSize, 0);
        cv::initUndistortRectifyMap(intrinsic_matrix[0],distortion_coeffs[0], R, optMatrix, undistorSize, CV_32FC1, mapx[0], mapy[0]);
        gpuMapx[0] = cv::cuda::GpuMat(mapx[0].clone());
        gpuMapy[0] = cv::cuda::GpuMat(mapy[0].clone());
        
        image_size = cv::Size(960, 540);
        undistorSize = image_size;
        mapx[1] = cv::Mat(undistorSize,CV_32FC1);
        mapy[1] = cv::Mat(undistorSize,CV_32FC1);
        optMatrix = getOptimalNewCameraMatrix(intrinsic_matrix[1], distortion_coeffs[1], image_size, 1, undistorSize, 0);
        cv::initUndistortRectifyMap(intrinsic_matrix[1],distortion_coeffs[1], R, optMatrix, undistorSize, CV_32FC1, mapx[1], mapy[1]);
        gpuMapx[1] = cv::cuda::GpuMat(mapx[1]);
        gpuMapy[1] = cv::cuda::GpuMat(mapy[1]);

        setDistoredSize(m_undistoredWidth);
        spdlog::info("!!!!!![{}] cam init ok!!!!!!!!!\n", m_id);

        sdkCreateTimer(&timer);
        sdkResetTimer(&timer);
        sdkStartTimer(&timer);

        printf("m_gpuDistoredImg:%p, m_gpuUndistoredImg:%p, m_gpuargb:%p\n", m_gpuDistoredImg.data, m_gpuUndistoredImg.data, m_gpuargb.data);
    }

    ~nvCam()
    {
        stop_stream(&ctx);
        if (ctx.cam_fd > 0)
        close(ctx.cam_fd);

        if (ctx.renderer != NULL)
            delete ctx.renderer;

        if (ctx.egl_display && !eglTerminate(ctx.egl_display))
            printf("Failed to terminate EGL display connection\n");

        if (ctx.g_buff != NULL)
        {
            for (unsigned i = 0; i < V4L2_BUFFERS_NUM; i++) {
                if (ctx.g_buff[i].dmabuff_fd)
                    NvBufferDestroy(ctx.g_buff[i].dmabuff_fd);
            }
            free(ctx.g_buff);
        }

        NvBufferDestroy(retNvbuf->dmabuff_fd);
        // NvBufferDestroy(retNvbuf[1].dmabuff_fd);
    }

    void setDistoredSize(int width)
    {
        changeszmtx.lock();
        while(!m_queue.empty())
            m_queue.pop();
        if(width == 1920)
        {
            m_undistoredWidth = 1920;
            m_undistoredHeight = 1080;
            distoredszIdx = 0;
        }
        else if(width == 960)
        {
            m_undistoredWidth = 960;
            m_undistoredHeight = 540;
            distoredszIdx = 1;
        }
        else
            spdlog::critical("invalid distored size");
        changeszmtx.unlock();

    }

    bool read_frame()
    {
        struct v4l2_buffer v4l2_buf;
        sdkResetTimer(&timer);
        /* Dequeue a camera buff */
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        v4l2_buf.memory = V4L2_MEMORY_DMABUF;
        if (ioctl(ctx.cam_fd, VIDIOC_DQBUF, &v4l2_buf) < 0)
            ERROR_RETURN("Failed to dequeue camera buff: %s (%d)",
                    strerror(errno), errno);

        ctx.frame++;
        NvBufferMemSyncForDevice(ctx.g_buff[v4l2_buf.index].dmabuff_fd, 0,
                (void**)&ctx.g_buff[v4l2_buf.index].start);

        /************* latency test *************/
        // if (-1 == NvBufferTransform(ctx.g_buff[v4l2_buf.index].dmabuff_fd, ctx.render_dmabuf_fd,
        //                     &transParams))
        //         ERROR_RETURN("Failed to convert the buffer");
        // ctx.renderer->render(ctx.render_dmabuf_fd);

        /*  Convert the camera buffer from YUV422 to ARGB */
        if (-1 == NvBufferTransform(ctx.g_buff[v4l2_buf.index].dmabuff_fd, retNvbuf->dmabuff_fd, &transParams))
            ERROR_RETURN("Failed to convert the buffer");

        // if(-1 == NvBuffer2Raw(retNvbuf[distoredszIdx].dmabuff_fd, 0, m_distoredWidth, m_distoredHeight, m_argb[distoredszIdx].data))
        //         ERROR_RETURN("Failed to NvBuffer2Raw");
        if(-1 == NvBuffer2Raw(retNvbuf->dmabuff_fd, 0, m_argb.size().width, m_argb.size().height, m_argb.data))
                ERROR_RETURN("Failed to NvBuffer2Raw");
        
        spdlog::trace("before undistored takes :{} ms\n", sdkGetTimerValue(&timer));

        
        if(m_undistor)
        {
            /***** cpu undistor *****/
            // cv::cvtColor(m_argb, m_ret, cv::COLOR_RGBA2RGB);
            cv::Mat tmp;
            cv::resize(m_argb, tmp, cv::Size(m_undistoredWidth, m_undistoredHeight));
            cv::cvtColor(tmp, tmp, cv::COLOR_RGBA2RGB);
            // m_distoredImg = tmp.clone();
            // // /*undistored*********/

            spdlog::trace("read frame before remap takes :{} ms", sdkGetTimerValue(&timer));
            cv::remap(tmp, m_undistoredImg, mapx[distoredszIdx], mapy[distoredszIdx], cv::INTER_CUBIC);

            spdlog::trace("read frame before cut and resize takes :{} ms", sdkGetTimerValue(&timer));
            m_undistoredImg = m_undistoredImg(cv::Rect(rectPara[distoredszIdx][0], rectPara[distoredszIdx][1], rectPara[distoredszIdx][2], rectPara[distoredszIdx][3]));
            cv::resize(m_undistoredImg, m_ret, cv::Size(m_retWidth, m_retHeight));
            
            /***** cpu undistor end*****/
        }
        else
        {
            cv::resize(m_argb, m_ret, cv::Size(m_retWidth, m_retHeight));
        }


        
        /***** gpu undistor *****/
        // m_gpuargb.upload(m_argb);
        // cv::cuda::resize(m_gpuargb, m_gpuDistoredImg, cv::Size(m_undistoredWidth, m_undistoredHeight));
        // // spdlog::trace("read frame before remap takes :{} ms", sdkGetTimerValue(&timer));
        // cv::cuda::remap(m_gpuDistoredImg, m_gpuUndistoredImg, gpuMapx[distoredszIdx], gpuMapy[distoredszIdx], cv::INTER_CUBIC);
        // m_gpuUndistoredImg = m_gpuDistoredImg(cv::Rect(rectPara[distoredszIdx][0], rectPara[distoredszIdx][1], rectPara[distoredszIdx][2], rectPara[distoredszIdx][3]));
        // cv::cuda::resize(m_gpuUndistoredImg, m_gpuret, cv::Size(m_retWidth, m_retHeight));
        // m_gpuret.download(m_ret);

        // cv::imshow("img"+std::to_string(m_id), m_argb);
        // cv::waitKey(1);
        // // m_ret = m_ret(cv::Rect(rectPara[distoredszIdx][0], rectPara[distoredszIdx][1], rectPara[distoredszIdx][2], rectPara[distoredszIdx][3]));

        // /***** gpu undistor end *****/

        // // cv::cuda::resize(m_gpuargb, m_gpuret, cv::Size(m_retWidth, m_retHeight));
        // // m_gpuret.download(m_ret);
        /***** gpu undistor end*****/

        spdlog::trace("undistor takes :{} ms", sdkGetTimerValue(&timer));

        // cv::resize(m_argb, m_ret, cv::Size(m_retWidth, m_retHeight));
        // Raw2NvBuffer(m_ret.data, 0, m_retWidth, m_retHeight, ctx.render_dmabuf_fd);
        // Raw2NvBuffer(m_argb[distoredszIdx].data, 0, m_retWidth, m_retHeight, ctx.render_dmabuf_fd);

        /******* render_dmabuf_fd to argb ok *******/
        // cv::Mat mtargb(1080,1920,CV_8UC4);
        // if(-1 == NvBuffer2Raw(ctx.render_dmabuf_fd, 0, 1920, 1080, mtargb.data))
        //         ERROR_RETURN("Failed to NvBuffer2Raw");
        // cv::imshow("argb", mtargb);
        // cv::waitKey(0);
        /******* render_dmabuf_fd to argb end *******/

        /******* g_buff to argb ok *******/
        // cv::Mat mt422(1080,1920,CV_8UC2);
        // memcpy(mt422.data, ctx.g_buff[v4l2_buf.index].start, 1920 * 1080 * 2 * sizeof(unsigned char));
        // cv::cvtColor(mt422, rgbii, cv::COLOR_YUV2BGR_YUYV);
        /******* g_buff to argb end *******/

        // cv::Mat mt420(1080*3/2,1920,CV_8UC1);
        
        // unsigned char *buf = (unsigned char*)malloc(1920*1080*3/2);

        // if(-1 == NvBuffer2Raw(ctx.render_dmabuf_fd, 0, 1920, 1080*3/2, mt420.data))
        //         ERROR_RETURN("Failed to NvBuffer2Raw");
        
        // // if(-1 == NvBuffer2Raw(ctx.render_dmabuf_fd, 0, 1920, 1080*3/2, buf))
        // //         ERROR_RETURN("Failed to NvBuffer2Raw");
        // // memcpy(mt422.data, ctx.g_buff[v4l2_buf.index].start, 1920 * 1080 * 2 * sizeof(unsigned char));
        // // memcpy(mt420.data, buf, 1920*1080*3/2 * sizeof(unsigned char));

        // cv::Mat rgbi, rgbii;
        // cv::cvtColor(mt420, rgbi, cv::COLOR_YUV2BGR_I420);
        // cv::imshow("420", rgbi);
        // cv::waitKey(0);




        /* Preview */
        // ctx.renderer->render(ctx.render_dmabuf_fd);

            // /* Enqueue camera buffer back to driver */
            // if (ioctl(ctx.cam_fd, VIDIOC_QBUF, &v4l2_buf))
            //     ERROR_RETURN("Failed to queue camera buffers: %s (%d)",
            //             strerror(errno), errno);

            // ctx.renderer->render(retNvbuf[distoredszIdx].dmabuff_fd);

            // if(-1 == NvBuffer2Raw(retNvbuf[distoredszIdx].dmabuff_fd, 0, m_distoredWidth, m_distoredHeight, m_argb[distoredszIdx].data))
            //     ERROR_RETURN("Failed to NvBuffer2Raw");

            // spdlog::trace("read frame before undistor takes :{} ms", sdkGetTimerValue(&timer));

            // cv::resize(m_argb[distoredszIdx], m_ret, cv::Size(m_distoredWidth, m_distoredHeight));
            /***** gpu undistor *****/
            // m_gpuargb[distoredszIdx].upload(m_argb[distoredszIdx]);
            // cv::cuda::resize(m_gpuargb[distoredszIdx], m_gpuDistoredImg, cv::Size(m_distoredWidth, m_distoredHeight));
            
            // spdlog::trace("read frame before remap takes :{} ms", sdkGetTimerValue(&timer));
            // cv::cuda::remap(m_gpuDistoredImg, m_gpuUndistoredImg, gpuMapx[distoredszIdx], gpuMapy[distoredszIdx], cv::INTER_CUBIC);
            
            // spdlog::trace("read frame before cut and resize takes :{} ms", sdkGetTimerValue(&timer));
            // m_gpuUndistoredImg = m_gpuUndistoredImg(cv::Rect(rectPara[distoredszIdx][0], rectPara[distoredszIdx][1], rectPara[distoredszIdx][2], rectPara[distoredszIdx][3]));
            // cv::cuda::resize(m_gpuUndistoredImg, m_gpuUndistoredImg, cv::Size(m_retWidth, m_retHeight));
            // m_gpuUndistoredImg.download(m_ret);
            /***** gpu undistor end *****/

            /***** cpu undistor *****/
            // cv::cvtColor(m_argb, m_ret, cv::COLOR_RGBA2RGB);
            // cv::Mat tmp;
            // cv::resize(m_argb[distoredszIdx], m_ret, cv::Size(m_distoredWidth, m_distoredHeight));
            // cv::resize(m_argb[distoredszIdx], tmp, cv::Size(m_distoredWidth, m_distoredHeight));
            // // cv::cvtColor(tmp, tmp, cv::COLOR_RGBA2RGB);
            // // m_distoredImg = tmp.clone();
            // // // /*undistored*********/

            // spdlog::trace("read frame before remap takes :{} ms", sdkGetTimerValue(&timer));
            // cv::remap(tmp, m_undistoredImg, mapx[distoredszIdx], mapy[distoredszIdx], cv::INTER_CUBIC);

            // spdlog::trace("read frame before cut and resize takes :{} ms", sdkGetTimerValue(&timer));
            // m_undistoredImg = m_undistoredImg(cv::Rect(rectPara[distoredszIdx][0], rectPara[distoredszIdx][1], rectPara[distoredszIdx][2], rectPara[distoredszIdx][3]));
            // cv::resize(m_undistoredImg, m_ret, cv::Size(m_retWidth, m_retHeight));
            
            /***** cpu undistor end*****/

            // spdlog::trace("read frame before VIDIOC_QBUF takes :{} ms", sdkGetTimerValue(&timer));
            /* Enqueue camera buffer back to driver */
            if (ioctl(ctx.cam_fd, VIDIOC_QBUF, &v4l2_buf))
                ERROR_RETURN("Failed to queue camera buffers: %s (%d)",
                        strerror(errno), errno);
            
            // printf("read_frame ctx.cam_fd:%d ok!!!\n", ctx.cam_fd);

            spdlog::trace("read frame takes :{} ms\n", sdkGetTimerValue(&timer));

            return true;
        
    }
    
    int getSrcFrame(cv::Mat &frame)
    {
        // if(getFrame())
        // {
        //     frame = m_distoredImg.clone();
        //     return RET_OK;
        // }
        // else
        //     return RET_ERR;
    
        return getFrame(frame);
    }

    void run()
	{
        // struct pollfd fds[1];
        // fds[0].fd = ctx->cam_fd;
        // fds[0].events = POLLIN;
		while(1)
		{
            // usleep(10);
			// printf("start run\n");
			if(!read_frame())
            {
                spdlog::critical("run fct read frame fail");
                continue;
            }
			std::unique_lock<std::mutex> lock(m_mtx[m_id]);
			while(m_queue.size() >= 10)
            {
                spdlog::warn("cam:[{}] wait for consumer", m_id);
				// m_queue.pop_front();
                con[m_id].wait(lock);
            }
            if(m_ret.size().width > 0)
            {
			    m_queue.push(m_ret);
            }
            con[m_id].notify_all();
			// printf("run ok!!!!!!!!!!!!1\n");
		}
	}

    int getFrame(cv::Mat &mat)
	{
        std::unique_lock<std::mutex> lock(m_mtx[m_id]);
        while(m_queue.empty())
        {
            spdlog::warn("cam:[{}] wait for img", m_id);
            // return 0;
            con[m_id].wait(lock);
        }
        mat = m_queue.front().clone();
        m_queue.pop();
        con[m_id].notify_all();

        return 1;
	}

public:
    camcontext_t ctx;

    int m_camSrcWidth, m_camSrcHeight;
	int m_retWidth, m_retHeight;
	int m_distoredWidth, m_distoredHeight;
	int m_undistoredWidth, m_undistoredHeight;
    int m_fd_video;
    char m_dev_name[30];
    cv::Mat m_argb, m_distoredImg, m_undistoredImg, m_ret, m_tmp;
	int m_id;
    bool m_withid;

    nv_buffer *retNvbuf;//for 1920 and 960
    NvBufferTransformParams transParams;

    std::queue<cv::Mat> m_queue;

    cv::Mat mapx[2], mapy[2];
    int distoredszIdx;

    StopWatchInterface *timer;

    cv::cuda::GpuMat gpuMapx[2], gpuMapy[2];
    cv::cuda::GpuMat m_gpuargb, m_gpuDistoredImg, m_gpuUndistoredImg;
    cv::cuda::GpuMat m_gpuret;

    bool m_undistor;
};