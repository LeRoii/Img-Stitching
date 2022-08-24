#include <chrono>

#include "nvrenderAlpha.h"

static int offsetX, offsetY, h, w;
static double fitscale;

nvrenderAlpha::nvrenderAlpha(const nvrenderCfg &cfg):nvrenderbase(cfg)
{
    // drawIndicator();
    canvas.setTo(0);
}

nvrenderAlpha::~nvrenderAlpha()
{

}

void nvrenderAlpha::drawIndicator()
{
    canvas.setTo(0);
    indicatorStartX = 10;
    longStartX = 0;
    uplongStartY = 20;
    uplongLen = 20;
    upshortLen = 10;
    uplongEndY = uplongStartY + uplongLen;
    upshortEndY = uplongStartY + upshortLen;
    longStep = 70;
    shortStep = longStep/5;

    double fontScale = 0.6;
    int lineSickness = 2;
    int fontSickness = 2;
    cv::Scalar color = cv::Scalar(5, 217, 82 );

    for(int i=0;i<19;i++)
    {
        longStartX = indicatorStartX+i*longStep;
        cv::line(canvas, cv::Point(longStartX, uplongStartY), cv::Point(longStartX, uplongEndY), color, lineSickness);
        if(i==0)
            cv::putText(canvas, std::to_string(i*20), cv::Point(longStartX-5, uplongStartY-10), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        else
            cv::putText(canvas, std::to_string(i*20), cv::Point(longStartX-20, uplongStartY-10), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        if(i == 18)
            continue;
        for(int j=0;j<4;j++)
            cv::line(canvas, cv::Point(longStartX+shortStep*(1+j), uplongStartY), cv::Point(longStartX+shortStep*(1+j), upshortEndY), color, lineSickness);
    }

    panoMargin = 5;
    panoHeight = 240;
    panoWidth = longStep*18;
    downlongStartY = uplongEndY + panoHeight + panoMargin*2;
    downlongEndY = downlongStartY + uplongLen;
    downshortEndY = downlongStartY + upshortLen;

    for(int i=0;i<19;i++)
    {
        longStartX = indicatorStartX+i*longStep;
        cv::line(canvas, cv::Point(longStartX, downlongStartY), cv::Point(longStartX, downlongEndY), color, lineSickness);
        if(i==0)
            cv::putText(canvas, std::to_string(i*20), cv::Point(longStartX-10, downlongEndY+30), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        else
            cv::putText(canvas, std::to_string(i*20), cv::Point(longStartX-20, downlongEndY+30), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        if(i == 18)
            continue;
        for(int j=0;j<4;j++)
            cv::line(canvas, cv::Point(longStartX+shortStep*(1+j), downlongStartY), cv::Point(longStartX+shortStep*(1+j), downshortEndY), color, lineSickness);
    }

    maxWidth = 18*102;
    maxHeight = downlongStartY - uplongEndY - 5;
}

cv::Mat nvrenderAlpha::renderegl(cv::Mat &img)
{
    cv::Mat tmp;
    cv::cvtColor(img, tmp, cv::COLOR_RGB2RGBA);
    fit2final(tmp);
    if(0 != NvBufferMemSyncForDevice (nvbufferfd, 0, (void**)&canvas.data))
        spdlog::warn("NvBufferMemSyncForDevice failed");
    renderer->render(nvbufferfd);

    return canvas;
}

cv::Mat nvrenderAlpha::renderocv(cv::Mat &img)
{
    fit2final(img);
    cv::imshow("final", canvas);
    return canvas;

    // cv::waitKey(1);
}

cv::Mat nvrenderAlpha::render(cv::Mat &img)
{
    // std::time_t tt = std::chrono::system_clock::to_time_t (std::chrono::system_clock::now());
    // // struct std::tm * ptm = std::localtime(&tt);
    // std::stringstream ss;
    // ss << std::put_time(std::localtime(&tt), "%F-%H-%M-%S");
    // std::string str = ss.str();
    // // std::cout << "Now (local time): " << std::put_time(ptm,"%F-%H-%M-%S") << '\n';
    // cv::putText(img, str, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(230, 235, 232 ), 2);

    if(m_mode == RENDER_EGL)
        return renderegl(img);
    else if(m_mode == RENDER_NONE)
        return fit2final(img);
    else if(m_mode == RENDER_OCV)
        return renderocv(img);

    // cv::Mat tmp;
    // if(m_mode == RENDER_EGL)
    //     return renderegl(img);
    // else if(m_mode == RENDER_OCV)
    //     return renderocv(img);
}

// void nvrenderAlpha::render(cv::Mat &img, cv::Mat &final)
// {
//     std::time_t tt = std::chrono::system_clock::to_time_t (std::chrono::system_clock::now());
//     // struct std::tm * ptm = std::localtime(&tt);
//     std::stringstream ss;
//     ss << std::put_time(std::localtime(&tt), "%F-%H-%M-%S");
//     std::string str = ss.str();
//     // std::cout << "Now (local time): " << std::put_time(ptm,"%F-%H-%M-%S") << '\n';
//     cv::putText(img, str, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(230, 235, 232 ), 2);

//     if(m_mode == RENDER_EGL)
//         renderegl(img);
//     else if(m_mode == RENDER_OCV)
//         renderocv(img, final);
// }

void nvrenderAlpha::showImg(cv::Mat &img)
{
    cv::Mat tmp;
    cv::cvtColor(img, tmp, cv::COLOR_RGB2RGBA);
    tmp.copyTo(canvas(cv::Rect(0, 0, nvbufferWidth, nvbufferHeight)));
    if(0 != NvBufferMemSyncForDevice (nvbufferfd, 0, (void**)&canvas.data))
        spdlog::warn("NvBufferMemSyncForDevice failed");
    renderer->render(nvbufferfd);
}

void nvrenderAlpha::renderimgs(cv::Mat &img, cv::Mat &inner, int x, int y)
{
    inner.copyTo(img(cv::Rect(x, y, 300, 300)));
    render(img);
}

cv::Mat nvrenderAlpha::fit2final(cv::Mat &input)
{
    
    if(input.cols == 1920 && input.rows == 1080)
    {
        input.copyTo(canvas); 
        return input;
    }
    // int offsetX, offsetY, h, w;
    static bool fitsizeok = false;
    cv::Mat tmp;
    if(!fitsizeok)
    {
        maxHeight = nvbufferHeight;
        maxWidth = nvbufferWidth;
        fitscale = 1;
        if(input.cols > maxWidth)
        {
            fitscale = maxWidth * 1.0 / input.cols;
        }
        // fitscale = maxWidth * 1.0 / input.cols;
        cv::resize(input, tmp, cv::Size(), fitscale, fitscale);
        w = tmp.cols;
        h = tmp.rows;
        // offsetX = indicatorStartX + (maxWidth - w)/2;
        // offsetY = uplongEndY + (maxHeight - h)/2;
        offsetX =  (maxWidth - w)/2;
        offsetY =  (maxHeight - h)/2;

        fitsizeok = true;

        spdlog::debug("fit2final w:{},h:{}, offsetX:{}, offsetY:{}", w, h, offsetX, offsetY);
    }
    cv::resize(input, tmp, cv::Size(), fitscale, fitscale);
    tmp.copyTo(canvas(cv::Rect(offsetX, offsetY, w, h)));

    return canvas;
}