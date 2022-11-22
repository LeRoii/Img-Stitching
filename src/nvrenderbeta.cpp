#include <chrono>

#include "nvrenderbeta.h"

static int offsetX, offsetY, h, w;
static double fitscale;

nvrenderBeta::nvrenderBeta(const stNvrenderCfg &cfg):nvrenderbase(cfg)
{
    drawIndicator();
}

nvrenderBeta::~nvrenderBeta()
{

}

void nvrenderBeta::drawIndicator()
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

    cv::Point center = cv::Point(960, 540);
    int radius = 130;
    float cathetus = radius * 1.0 / sqrt(2);
    cv::Point leftTop = center + cv::Point(-cathetus, -cathetus);
    cv::Point leftBot = center + cv::Point(-cathetus, cathetus);
    cv::Point rightBot = center + cv::Point(cathetus, cathetus);
    cv::Point rightTop = center + cv::Point(cathetus, -cathetus);

    cv::circle(canvas, center, radius, cv::Scalar(0, 255, 0), 1);
    cv::line(canvas, center, leftTop, cv::Scalar(255, 0, 0), 1);
    cv::line(canvas, center, rightTop, cv::Scalar(255, 0, 0), 1);

    cv::putText(canvas, "camera No.1", center + cv::Point(-60, -160), cv::FONT_HERSHEY_SIMPLEX, 0.6, color, fontSickness);


    maxWidth = 18*102;
    maxHeight = downlongStartY - uplongEndY - 5;
}

cv::Mat nvrenderBeta::renderegl(cv::Mat &img)
{
    cv::Mat tmp;
    cv::cvtColor(img, tmp, cv::COLOR_RGB2RGBA);
    // fit2final(tmp, canvas);
    if(0 != NvBufferMemSyncForDevice (nvbufferfd, 0, (void**)&canvas.data))
        spdlog::warn("NvBufferMemSyncForDevice failed");
    renderer->render(nvbufferfd);

    return canvas;
}

cv::Mat nvrenderBeta::renderocv(cv::Mat &img)
{
    fit2final(img, canvas);
    cv::imshow("final", canvas);
    // cv::waitKey(1);
    return canvas;
}


cv::Mat nvrenderBeta::render(cv::Mat &img)
{
    std::time_t tt = std::chrono::system_clock::to_time_t (std::chrono::system_clock::now());
    // struct std::tm * ptm = std::localtime(&tt);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&tt), "%F-%H-%M-%S");
    std::string str = ss.str();
    // std::cout << "Now (local time): " << std::put_time(ptm,"%F-%H-%M-%S") << '\n';
    cv::putText(img, str, cv::Point(20, 30), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(230, 235, 232 ), 2);

    if(m_mode == RENDER_EGL)
        return renderegl(img);
    else if(m_mode == RENDER_OCV)
        return renderocv(img);
}

void nvrenderBeta::showImg(cv::Mat &img)
{
    cv::Mat tmp;
    cv::cvtColor(img, tmp, cv::COLOR_RGB2RGBA);
    tmp.copyTo(canvas(cv::Rect(0, 0, nvbufferWidth, nvbufferHeight)));
    if(0 != NvBufferMemSyncForDevice (nvbufferfd, 0, (void**)&canvas.data))
        spdlog::warn("NvBufferMemSyncForDevice failed");
    renderer->render(nvbufferfd);
}

void nvrenderBeta::renderimgs(cv::Mat &img, cv::Mat &inner, int x, int y)
{
    inner.copyTo(img(cv::Rect(x, y, 300, 300)));
    render(img);
}

void nvrenderBeta::fit2final(cv::Mat &input, cv::Mat &output)
{
    if(input.cols == nvbufferWidth && input.rows == nvbufferHeight)
    {
        input.copyTo(output); 
        return;
    }
    // int offsetX, offsetY, h, w; 
    static bool fitsizeok = false;
    cv::Mat tmp;
    if(!fitsizeok)
    {
        fitscale = 1;
        if(input.cols > maxWidth)
        {
            fitscale = maxWidth * 1.0 / input.cols;
        }
        cv::resize(input, tmp, cv::Size(), fitscale, fitscale);
        w = tmp.cols;
        h = tmp.rows;
        offsetX = indicatorStartX + (maxWidth - w)/2;
        offsetY = uplongEndY + (maxHeight - h)/2;

        fitsizeok = true;
    }
    cv::resize(input, tmp, cv::Size(), fitscale, fitscale);
    tmp.copyTo(output(cv::Rect(offsetX, offsetY, w, h)));
}

void nvrenderBeta::renderWithUi(cv::Mat &pano, cv::Mat &ori)
{
    cv::resize(pano, pano, cv::Size(panoWidth, panoHeight));
    pano.copyTo(canvas(cv::Rect(indicatorStartX, uplongEndY + 5, panoWidth, panoHeight)));
    ori.copyTo(canvas(cv::Rect(indicatorStartX, oriStartY + 20, ori.cols, ori.rows)));
    renderegl(canvas);
    // cv::imwrite("rendertest.png", canvas);

    return;

    // cv::Mat tmp;
    // if(m_mode == RENDER_EGL)
    //     renderegl(img);
    // else if(m_mode == RENDER_OCV)
    //     renderocv(img, tmp);
}