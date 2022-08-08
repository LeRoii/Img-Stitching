#include "nvrenderbase.h"

nvrenderbase::nvrenderbase(const nvrenderCfg &cfg):
nvbufferWidth(cfg.bufferw), nvbufferHeight(cfg.bufferh),m_mode(cfg.mode)
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
        renderer->setFPS(50);

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
        // cv::namedWindow("input", CV_WINDOW_AUTOSIZE);
        // cv::moveWindow("input",0,0);
        // cv::setWindowProperty("input", CV_WINDOW_FULLSCREEN, CV_WINDOW_FULLSCREEN);
    }

    spdlog::debug("renderbase ctor cplt");
}

nvrenderbase::~nvrenderbase()
{
    if(renderer != nullptr)
        {
            delete renderer;
            renderer = nullptr;
        }

    NvBufferDestroy(nvbufferfd);
}