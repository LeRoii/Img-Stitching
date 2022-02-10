#ifndef _NVRENDER_H_
#define _NVRENDER_H_

#include <opencv2/opencv.hpp>
#include "NvEglRenderer.h"
#include "nvbuf_utils.h"
#include "spdlog/spdlog.h"
#include "stitcherconfig.h"
class nvrender
{
public:
    nvrender(nvrenderCfg cfg):nvbufferWidth(cfg.bufferw), nvbufferHeight(cfg.bufferh)
    {
        NvBufferCreateParams bufparams = {0};
        bufparams.payloadType = NvBufferPayload_SurfArray;
        bufparams.width = cfg.bufferw;
        bufparams.height = cfg.bufferh;
        bufparams.layout = NvBufferLayout_Pitch;
        bufparams.colorFormat = NvBufferColorFormat_ARGB32;
        bufparams.nvbuf_tag = NvBufferTag_NONE;

        if (-1 == NvBufferCreateEx(&nvbufferfd, &bufparams))
                spdlog::critical("Failed to create NvBuffer nvbufferfd");
        
        renderer = NvEglRenderer::createEglRenderer("renderer0", cfg.renderw, cfg.renderh, cfg.renderx, cfg.rendery);
        if(!renderer)
            spdlog::critical("Failed to create EGL renderer");
        renderer->setFPS(30);

        canvas = cv::Mat(cfg.bufferh, cfg.bufferw, CV_8UC4);
        canvas.setTo(0);

        NvBufferParams params = {0};
        NvBufferGetParams (nvbufferfd, &params);
        spdlog::debug("params num planes:{}", params.num_planes);
        if(0 != NvBufferMemMap (nvbufferfd, 0, NvBufferMem_Read_Write, (void**)&canvas.data))
            spdlog::critical("NvBufferMemMap Failed");
        
        spdlog::info("rendereer ctor cplt");
        spdlog::info("rendereer ctor cplt,nvbufferWidth:{}", nvbufferWidth);
    }
    ~nvrender()
    {
        if(renderer != nullptr)
        {
            delete renderer;
            renderer = nullptr;
        }

        NvBufferDestroy(nvbufferfd);
    }

    void render(unsigned char *data)
    {
        Raw2NvBuffer(data, 0, nvbufferWidth, nvbufferHeight, nvbufferfd);
        renderer->render(nvbufferfd);
    }

    void render(cv::Mat &img)
    {
        int w = img.size().width;
        int h = img.size().height;
        int offsetx = (nvbufferWidth - w)/2;
        int offsety = (nvbufferHeight - h)/2;
        cv::Mat tmp;
        cv::cvtColor(img, tmp, cv::COLOR_RGB2RGBA);
        tmp.copyTo(canvas(cv::Rect(offsetx, offsety, w, h)));
        // img.copyTo(canvas);
        if(0 != NvBufferMemSyncForDevice (nvbufferfd, 0, (void**)&canvas.data))
            spdlog::warn("NvBufferMemSyncForDevice failed");
        renderer->render(nvbufferfd);

        
    }

    // void render1()
    // {
    //     spdlog::info("in render1");
    //     cv::Mat im = cv::imread("/home/nvidia/ssd/data/ori/3-ori7.png");
    //     cv::cvtColor(im,im,cv::COLOR_RGB2RGBA);
    //     spdlog::info("in render1, nvbufferfd:{}",nvbufferWidth);
    //     Raw2NvBuffer(im.data, 0, nvbufferWidth, nvbufferHeight, nvbufferfd);
    //     renderer->render(nvbufferfd);
    // }

private:
    NvEglRenderer *renderer;
    int nvbufferfd;
    int nvbufferWidth, nvbufferHeight;
    cv::Mat canvas;
};

#endif