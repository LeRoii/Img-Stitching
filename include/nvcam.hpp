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

#define LOG(msg) std::cout << msg
#define LOGLN(msg) std::cout << msg << std::endl

static bool quit = false;

int rectPara[4] = {36,53,888,440};
cv::Mat intrinsic_matrix[1];
cv::Mat distortion_coeffs[1];

using namespace std;

std::mutex m_mtx[8];
std::condition_variable con[8];

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
save_frame_to_file(camcontext_t * ctx, struct v4l2_buffer * buf)
{
    int file;

    file = open(ctx->cam_file, O_CREAT | O_WRONLY | O_APPEND | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (-1 == file)
        ERROR_RETURN("Failed to open file for frame saving");

    if (-1 == write(file, ctx->g_buff[buf->index].start,
                ctx->g_buff[buf->index].size))
    {
        close(file);
        ERROR_RETURN("Failed to write frame into file");
    }

    close(file);

    return true;
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
display_initialize(camcontext_t * ctx)
{
    /* Create EGL renderer */
    ctx->renderer = NvEglRenderer::createEglRenderer("renderer0",
            ctx->cam_w/3, ctx->cam_h/3, 0, 0);
    if (!ctx->renderer)
        ERROR_RETURN("Failed to create EGL renderer");
    ctx->renderer->setFPS(ctx->fps);

    if (ctx->enable_cuda)
    {
        /* Get defalut EGL display */
        ctx->egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (ctx->egl_display == EGL_NO_DISPLAY)
            ERROR_RETURN("Failed to get EGL display connection");

        /* Init EGL display connection */
        if (!eglInitialize(ctx->egl_display, NULL, NULL))
            ERROR_RETURN("Failed to initialize EGL display connection");
    }

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
    rb.count = V4L2_BUFFERS_NUM;
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
    input_params.nvbuf_tag = NvBufferTag_NONE;
    // /* Create Render buffer */
    if (-1 == NvBufferCreateEx(&ctx->render_dmabuf_fd, &input_params))
        ERROR_RETURN("Failed to create NvBuffer");

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
	m_distoredWidth(camcfg.distoredWidth),m_distoredHeight(camcfg.distoredHeight), m_id(camcfg.id)
    {

        intrinsic_matrix[0] = (cv::Mat_<double>(3,3) << 853.417882746302, 0, 483.001902270090,
                        0, 959.666714085956, 280.450178308760,
                        0, 0, 1);

		distortion_coeffs[0] = (cv::Mat_<double>(1,4) << -0.368584528301156, 0.0602436114872144, 0, 0);

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

        NvBufferCreateParams bufparams = {0};
        retNvbuf = (nv_buffer *)malloc(sizeof(nv_buffer));
        bufparams.payloadType = NvBufferPayload_SurfArray;
        bufparams.width = m_camSrcWidth;
        bufparams.height = m_camSrcHeight;
        bufparams.layout = NvBufferLayout_Pitch;
        bufparams.colorFormat = NvBufferColorFormat_ARGB32;
        bufparams.nvbuf_tag = NvBufferTag_CAMERA;
        if (-1 == NvBufferCreateEx(&retNvbuf->dmabuff_fd, &bufparams))
                spdlog::critical("Failed to create NvBuffer");

        m_argb = cv::Mat(bufparams.height, bufparams.width, CV_8UC4);

        /* Init the NvBufferTransformParams */
        memset(&transParams, 0, sizeof(transParams));
        transParams.transform_flag = NVBUFFER_TRANSFORM_FILTER;
        transParams.transform_filter = NvBufferTransform_Filter_Smart;

        // m_queue.resize(10);

        cv::Size image_size = cv::Size(m_distoredWidth, m_distoredHeight);
        cv::Size undistorSize = image_size;
        mapx = cv::Mat(undistorSize,CV_32FC1);
        mapy = cv::Mat(undistorSize,CV_32FC1);
        cv::Mat R = cv::Mat::eye(3,3,CV_32F);
        cv::Mat optMatrix = getOptimalNewCameraMatrix(intrinsic_matrix[0], distortion_coeffs[0], image_size, 1, undistorSize, 0);
        cv::initUndistortRectifyMap(intrinsic_matrix[0],distortion_coeffs[0], R, optMatrix, undistorSize, CV_32FC1, mapx, mapy);

        spdlog::info("!!!!!![{}] cam init ok!!!!!!!!!\n", m_id);
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

        // NvBufferDestroy(ctx.render_dmabuf_fd);
    }

    // bool start_capture()
    // {
    //     struct sigaction sig_action;
    //     struct pollfd fds[1];
    //     NvBufferTransformParams transParams;

    //     /* Register a shuwdown handler to ensure
    //     a clean shutdown if user types <ctrl+c> */
    //     sig_action.sa_handler = signal_handle;
    //     sigemptyset(&sig_action.sa_mask);
    //     sig_action.sa_flags = 0;
    //     sigaction(SIGINT, &sig_action, NULL);

    //     // if (ctx->cam_pixfmt == V4L2_PIX_FMT_MJPEG)
    //     //     ctx->jpegdec = NvJPEGDecoder::createJPEGDecoder("jpegdec");

    //     /* Init the NvBufferTransformParams */
    //     memset(&transParams, 0, sizeof(transParams));
    //     transParams.transform_flag = NVBUFFER_TRANSFORM_FILTER;
    //     transParams.transform_filter = NvBufferTransform_Filter_Smart;

    //     /* Enable render profiling information */
    //     // ctx.renderer->enableProfiling();

    //     fds[0].fd = ctx.cam_fd;
    //     fds[0].events = POLLIN;

    //     /* read a raw YUYV data from disk and display*/
    //     // int img = open("../camera.YUYV", O_RDONLY);
    //     // if (-1 == img)
    //     //     ERROR_RETURN("Failed to open file for rendering");
    //     // int bufsize = 3840*2160*2;
    //     // unsigned char *buf = (unsigned char*)malloc(bufsize);
    //     // int cnt = read(img, buf, bufsize);
    //     //     printf("read %d bytes\n", cnt);

    //     // NvBufferCreateParams bufparams = {0};
    //     // nv_buffer *nvbuf = (nv_buffer *)malloc(sizeof(nv_buffer));
    //     // bufparams.payloadType = NvBufferPayload_SurfArray;
    //     // bufparams.width = ctx.cam_w;
    //     // bufparams.height = ctx.cam_h;
    //     // bufparams.layout = NvBufferLayout_Pitch;
    //     // bufparams.colorFormat = get_nvbuff_color_fmt(V4L2_PIX_FMT_YUYV);
    //     // bufparams.nvbuf_tag = NvBufferTag_CAMERA;
    //     // if (-1 == NvBufferCreateEx(&nvbuf->dmabuff_fd, &bufparams))
    //     //         ERROR_RETURN("Failed to create NvBuffer");
        
    //     // if(-1 == Raw2NvBuffer(buf, 0, 3840, 2160, nvbuf->dmabuff_fd))
    //     //         ERROR_RETURN("Failed to Raw2NvBuffer");
        
    //     // // if (-1 == NvBufferTransform(nvbuf->dmabuff_fd, ctx.render_dmabuf_fd,
    //     // //                     &transParams))
    //     // //             ERROR_RETURN("Failed to convert the yuvvvv buffer");
        
    //     // // ctx.renderer->render(ctx.render_dmabuf_fd);

    //     // int rgbRender;
    //     // bufparams.colorFormat = NvBufferColorFormat_ARGB32;
    //     // bufparams.width = 960;
    //     // bufparams.height = 540;
    //     // if (-1 == NvBufferCreateEx(&rgbRender, &bufparams))
    //     //         ERROR_RETURN("Failed to create NvBuffer");
        
    //     // if (-1 == NvBufferTransform(nvbuf->dmabuff_fd, rgbRender,
    //     //                     &transParams))
    //     //             ERROR_RETURN("Failed to convert the yuvvvv buffer");
        
    //     // // while (poll(fds, 1, 5000) > 0 && !quit)
    //     // // {
    //     // //     ctx->renderer->render(rgbRender);
    //     // // }

    //     // unsigned char *rgbbuf = (unsigned char*)malloc(bufparams.width*bufparams.height*4);
    //     // if(-1 == NvBuffer2Raw(rgbRender, 0, bufparams.width, bufparams.height, rgbbuf))
    //     //     ERROR_RETURN("Failed to NvBuffer2Raw");
        
    //     // cv::Mat mtt(bufparams.height, bufparams.width, CV_8UC4, rgbbuf);
    //     // cv::imshow("a", mtt);
    //     // cv::waitKey(0);

    //     // return 0;

    //     /* Wait for camera event with timeout = 5000 ms */
    //     while (poll(fds, 1, 5000) > 0 && !quit)
    //     {
    //         if (fds[0].revents & POLLIN) {
    //             struct v4l2_buffer v4l2_buf;

    //             /* Dequeue a camera buff */
    //             memset(&v4l2_buf, 0, sizeof(v4l2_buf));
    //             v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //             if (ctx.capture_dmabuf)
    //                 v4l2_buf.memory = V4L2_MEMORY_DMABUF;
    //             else
    //                 v4l2_buf.memory = V4L2_MEMORY_MMAP;
    //             if (ioctl(ctx.cam_fd, VIDIOC_DQBUF, &v4l2_buf) < 0)
    //                 ERROR_RETURN("Failed to dequeue camera buff: %s (%d)",
    //                         strerror(errno), errno);

    //             ctx.frame++;

    //             if (ctx.capture_dmabuf) {
    //                 /* Cache sync for VIC operation since the data is from CPU */
    //                 NvBufferMemSyncForDevice(ctx.g_buff[v4l2_buf.index].dmabuff_fd, 0,
    //                         (void**)&ctx.g_buff[v4l2_buf.index].start);
    //             } else {
    //                 /* Copies raw buffer plane contents to an NvBuffer plane */
    //                 Raw2NvBuffer(ctx.g_buff[v4l2_buf.index].start, 0,
    //                             ctx.cam_w, ctx.cam_h, ctx.g_buff[v4l2_buf.index].dmabuff_fd);
    //             }

    //             /*  Convert the camera buffer from YUV422 to YUV420P */
    //             if (-1 == NvBufferTransform(ctx.g_buff[v4l2_buf.index].dmabuff_fd, ctx.render_dmabuf_fd,
    //                         &transParams))
    //                 ERROR_RETURN("Failed to convert the buffer");

    //             if (-1 == NvBufferTransform(ctx.g_buff[v4l2_buf.index].dmabuff_fd, retNvbuf->dmabuff_fd,
    //                         &transParams))
    //                 ERROR_RETURN("Failed to convert the yuvvvv buffer");

    //             if(-1 == NvBuffer2Raw(retNvbuf->dmabuff_fd, 0, m_distoredWidth, m_distoredHeight, m_ret.data))
    //                 ERROR_RETURN("Failed to NvBuffer2Raw");

    //             // cv::imshow("1", m_ret);
    //             // cv::waitKey(1);

    //             /* Preview */
    //             // ctx.renderer->render(ctx.render_dmabuf_fd);
    //             // ctx->renderer->render(nvbuf->dmabuff_fd);
    //             // ctx->renderer->render(img);

    //             /* Enqueue camera buffer back to driver */
    //             if (ioctl(ctx.cam_fd, VIDIOC_QBUF, &v4l2_buf))
    //                 ERROR_RETURN("Failed to queue camera buffers: %s (%d)",
    //                         strerror(errno), errno);
    //         }
    //     }

    //     /* Print profiling information when streaming stops */
    //     // ctx->renderer->printProfilingStats();


    //     return true;
    // }

    bool read_frame()
    {
        struct pollfd fds[1];
        fds[0].fd = ctx.cam_fd;
        fds[0].events = POLLIN;

        struct v4l2_buffer v4l2_buf;

        /* Dequeue a camera buff */
        memset(&v4l2_buf, 0, sizeof(v4l2_buf));
        v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        // if (ctx.capture_dmabuf)
        //     v4l2_buf.memory = V4L2_MEMORY_DMABUF;
        // else
        //     v4l2_buf.memory = V4L2_MEMORY_MMAP;
        
        // if (ioctl(ctx.cam_fd, VIDIOC_DQBUF, &v4l2_buf) < 0)
        //     ERROR_RETURN("Failed to dequeue camera buff: %s (%d)",
        //             strerror(errno), errno);

        // ctx.frame++;

        // if (ctx.capture_dmabuf) {
        //     /* Cache sync for VIC operation since the data is from CPU */
        //     NvBufferMemSyncForDevice(ctx.g_buff[v4l2_buf.index].dmabuff_fd, 0,
        //             (void**)&ctx.g_buff[v4l2_buf.index].start);
        // } else {
        //     /* Copies raw buffer plane contents to an NvBuffer plane */
        //     Raw2NvBuffer(ctx.g_buff[v4l2_buf.index].start, 0,
        //                 ctx.cam_w, ctx.cam_h, ctx.g_buff[v4l2_buf.index].dmabuff_fd);
        // }
        v4l2_buf.memory = V4L2_MEMORY_DMABUF;
        
        if (ioctl(ctx.cam_fd, VIDIOC_DQBUF, &v4l2_buf) < 0)
            ERROR_RETURN("Failed to dequeue camera buff: %s (%d)",
                    strerror(errno), errno);

        ctx.frame++;

        /* Cache sync for VIC operation since the data is from CPU */
        NvBufferMemSyncForDevice(ctx.g_buff[v4l2_buf.index].dmabuff_fd, 0,
                (void**)&ctx.g_buff[v4l2_buf.index].start);

        /*  Convert the camera buffer from YUV422 to YUV420P */
        // if (-1 == NvBufferTransform(ctx.g_buff[v4l2_buf.index].dmabuff_fd, ctx.render_dmabuf_fd,
        //             &transParams))
        //     ERROR_RETURN("Failed to convert the buffer");

        if (-1 == NvBufferTransform(ctx.g_buff[v4l2_buf.index].dmabuff_fd, retNvbuf->dmabuff_fd,
                    &transParams))
            ERROR_RETURN("Failed to convert the yuvvvv buffer");

        if(-1 == NvBuffer2Raw(retNvbuf->dmabuff_fd, 0, m_camSrcWidth, m_camSrcHeight, m_argb.data))
            ERROR_RETURN("Failed to NvBuffer2Raw");

        // cv::cvtColor(m_argb, m_ret, cv::COLOR_RGBA2RGB);

        cv::resize(m_argb, m_distoredImg, cv::Size(m_distoredWidth, m_distoredHeight));
        cv::cvtColor(m_distoredImg, m_distoredImg, cv::COLOR_RGBA2RGB);
        // /*undistored*********/
        cv::remap(m_distoredImg,m_distoredImg,mapx, mapy, cv::INTER_CUBIC);
        m_distoredImg = m_distoredImg(cv::Rect(rectPara[0],rectPara[1],rectPara[2],rectPara[3]));
        cv::resize(m_distoredImg, m_ret, cv::Size(m_retWidth, m_retHeight));
        
        
        /*undistored end*/

        // cv::imshow("1", m_ret);
        // cv::waitKey(1);

        // cv::imwrite("rf.png", m_ret);

        /* Enqueue camera buffer back to driver */
        if (ioctl(ctx.cam_fd, VIDIOC_QBUF, &v4l2_buf))
            ERROR_RETURN("Failed to queue camera buffers: %s (%d)",
                    strerror(errno), errno);
        
        // printf("read_frame ctx.cam_fd:%d ok!!!\n", ctx.cam_fd);

        return true;
    }
    
    int getSrcFrame(cv::Mat &frame)
    {
        if(read_frame())
        {
            frame = m_ret.clone();
            return RET_OK;
        }
        else
            return RET_ERR;
    }

    void run()
	{
		while(1)
		{
            // usleep(10);
			// printf("start run\n");
			read_frame();
			std::unique_lock<std::mutex> lock(m_mtx[m_id]);
			// m_mtx[m_id].lock();
			while(m_queue.size() >= 10)
				// m_queue.pop_front();
                con[m_id].wait(lock);
            if(m_ret.size().width > 0)
            {
			    m_queue.push(m_ret);
            }
			// m_mtx[m_id].unlock();
            con[m_id].notify_all();
			// printf("run ok!!!!!!!!!!!!1\n");
		}
	}

    int getFrame(cv::Mat &mat)
	{
        std::unique_lock<std::mutex> lock(m_mtx[m_id]);
        while(m_queue.empty())
        {
            // LOGLN("empty::"<<m_id);
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
    int m_fd_video;
    char m_dev_name[30];
    cv::Mat m_argb, m_distoredImg, m_ret, m_tmp;
	int m_id;

    nv_buffer * retNvbuf;
    NvBufferTransformParams transParams;

    std::queue<cv::Mat> m_queue;

    cv::Mat mapx, mapy;
    
    
};