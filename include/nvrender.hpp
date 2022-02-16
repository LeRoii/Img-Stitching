#ifndef _NVRENDER_H_
#define _NVRENDER_H_

#include <opencv2/opencv.hpp>
#include "NvEglRenderer.h"
#include "nvbuf_utils.h"
#include "spdlog/spdlog.h"
#include "stitcherconfig.h"

static int offsetX, offsetY, h, w;
static double fitscale;
class nvrender
{
public:
    nvrender(nvrenderCfg cfg):nvbufferWidth(cfg.bufferw), nvbufferHeight(cfg.bufferh),
    m_mode(cfg.mode)
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
        
        if(cfg.mode == RENDER_EGL)
        {
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
        }
        else if(cfg.mode == RENDER_OCV)
        {
            canvas = cv::Mat(cfg.bufferh, cfg.bufferw, CV_8UC3);
            canvas.setTo(0);
        }
        
        spdlog::debug("cfg.bufferh:{}, cfg.bufferw:{}",cfg.bufferh, cfg.bufferw);
        spdlog::info("render ctor cplt");
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

    void fit2final(cv::Mat &input, cv::Mat &output)
    {
        // int offsetX, offsetY, h, w; 
        static bool fitsizeok = false;
        spdlog::warn("fit2final start");
        cv::Mat tmp;
        if(!fitsizeok)
        {
            fitscale = 1;
            if(input.cols > 1920)
            {
                fitscale = 1920 * 1.0 / input.cols;
            }
            cv::resize(input, tmp, cv::Size(), fitscale, fitscale);
            w = tmp.cols;
            h = tmp.rows;
            offsetX = (1920 - w)/2;
            offsetY = (1080 - h)/2;

            fitsizeok = true;
        }
        cv::resize(input, tmp, cv::Size(), fitscale, fitscale);
        spdlog::warn("after fit2final resize");
        tmp.copyTo(output(cv::Rect(offsetX, offsetY, w, h)));
        spdlog::warn("after fit2final copyTo");
    }


    void renderegl(cv::Mat &img)
    {
        cv::Mat tmp;
        cv::cvtColor(img, tmp, cv::COLOR_RGB2RGBA);
        spdlog::warn("after cvtColor");
        fit2final(tmp, canvas);
        spdlog::warn("after fit2final");
        if(0 != NvBufferMemSyncForDevice (nvbufferfd, 0, (void**)&canvas.data))
            spdlog::warn("NvBufferMemSyncForDevice failed");
        renderer->render(nvbufferfd);
        spdlog::warn("after render");
    }

    void renderocv(cv::Mat &img)
    {
        fit2final(img, canvas);
        
        cv::imshow("final", canvas);
        cv::waitKey(1);
        spdlog::warn("after renderocv imshow");
    }

    void render(cv::Mat &img)
    {
        if(m_mode == RENDER_EGL)
            renderegl(img);
        else if(m_mode == RENDER_OCV)
            renderocv(img);
    }

private:
    NvEglRenderer *renderer;
    int nvbufferfd;
    int nvbufferWidth, nvbufferHeight;
    cv::Mat canvas;
    int m_mode;
};

#endif