#include <chrono>

#include "nvrenderAlpha.h"

static int offsetX, offsetY, h, w;
static double fitscale;

nvrenderAlpha::nvrenderAlpha(const nvrenderCfg &cfg):nvrenderbase(cfg)
{
    drawIndicator();
}

nvrenderAlpha::~nvrenderAlpha()
{

}

void nvrenderAlpha::drawIndicator()
{
    canvas.setTo(0);
    indicatorStartX = 30;
    longStartX = 0;
    uplongStartY = 200;
    uplongEndY = uplongStartY+30;
    upshortEndY = uplongStartY+15;
    double fontScale = 0.6;
    int lineSickness = 2;
    int fontSickness = 2;
    cv::Scalar color = cv::Scalar(5, 217, 82 );
    for(int i=0;i<19;i++)
    {
        longStartX = indicatorStartX+i*102;
        cv::line(canvas, cv::Point(longStartX, uplongStartY), cv::Point(longStartX, uplongEndY), color, lineSickness);
        // if(i==0)
        //     cv::putText(canvas, std::to_string(i*10), cv::Point(longStartX-10, uplongStartY-10), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        // else
        //     cv::putText(canvas, std::to_string(i*10), cv::Point(longStartX-20, uplongStartY-10), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        if(i == 18)
            continue;
        for(int j=0;j<4;j++)
            cv::line(canvas, cv::Point(longStartX+20*(1+j), uplongStartY), cv::Point(longStartX+20*(1+j), upshortEndY), color, lineSickness);
    }

    downlongStartY = 880;
    downlongEndY = downlongStartY+30;
    downshortEndY = downlongStartY+15;
    for(int i=0;i<19;i++)
    {
        longStartX = indicatorStartX+i*102;
        cv::line(canvas, cv::Point(longStartX, downlongStartY), cv::Point(longStartX, downlongEndY), color, lineSickness);
        // if(i==0)
        //     cv::putText(canvas, std::to_string(180+i*10), cv::Point(longStartX-10, downlongEndY+30), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        // else
        //     cv::putText(canvas, std::to_string(180+i*10), cv::Point(longStartX-20, downlongEndY+30), cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
        if(i == 18)
            continue;
        for(int j=0;j<4;j++)
            cv::line(canvas, cv::Point(longStartX+20*(1+j), downlongStartY), cv::Point(longStartX+20*(1+j), downshortEndY), color, lineSickness);
    }

    maxWidth = 18*102;
    maxHeight = downlongStartY - uplongEndY - 5;
}

cv::Mat nvrenderAlpha::renderegl(cv::Mat &img)
{
    cv::Mat tmp;
    cv::cvtColor(img, tmp, cv::COLOR_RGB2RGBA);
    fit2final(tmp, canvas);
    if(0 != NvBufferMemSyncForDevice (nvbufferfd, 0, (void**)&canvas.data))
        spdlog::warn("NvBufferMemSyncForDevice failed");
    renderer->render(nvbufferfd);

    return canvas;
}

cv::Mat nvrenderAlpha::renderocv(cv::Mat &img)
{
    fit2final(img, canvas);
    cv::imshow("final", canvas);
    return canvas;

    // cv::waitKey(1);
}

cv::Mat nvrenderAlpha::render(cv::Mat &img)
{
    std::time_t tt = std::chrono::system_clock::to_time_t (std::chrono::system_clock::now());
    // struct std::tm * ptm = std::localtime(&tt);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&tt), "%F-%H-%M-%S");
    std::string str = ss.str();
    // std::cout << "Now (local time): " << std::put_time(ptm,"%F-%H-%M-%S") << '\n';
    cv::putText(img, str, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(230, 235, 232 ), 2);

    cv::Mat tmp;
    if(m_mode == RENDER_EGL)
        return renderegl(img);
    else if(m_mode == RENDER_OCV)
        return renderocv(img);
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

void nvrenderAlpha::fit2final(cv::Mat &input, cv::Mat &output)
{
    if(input.cols == 1920 && input.rows == 1080)
    {
        input.copyTo(output); 
        return;
    }
    // int offsetX, offsetY, h, w; 
    static bool fitsizeok = false;
    cv::Mat tmp;
    if(!fitsizeok)
    {
        maxHeight = 720;
        maxWidth = 1280;
        fitscale = 1;
        if(input.cols > maxWidth)
        {
            fitscale = maxWidth * 1.0 / input.cols;
        }
        fitscale = maxWidth * 1.0 / input.cols;
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
    tmp.copyTo(output(cv::Rect(offsetX, offsetY, w, h)));
}