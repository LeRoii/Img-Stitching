#include "yaml-cpp/yaml.h"
#include "panocam.h"
#include "panoapp.h"
#include <termio.h>
#include <stdio.h>
#include <unistd.h>
#include <thread>


nvrender *pRenderer;
int g_keyboardinput = 0;
int g_usrcmd;

int scanKeyboard()
{
  //  struct termios
  //    {
  //      tcflag_t c_iflag;		/* input mode flags */
  //      tcflag_t c_oflag;		/* output mode flags */
  //      tcflag_t c_cflag;		/* control mode flags */
  //      tcflag_t c_lflag;		/* local mode flags */
  //      cc_t c_line;			/* line discipline */
  //      cc_t c_cc[NCCS];		/* control characters */
  //      speed_t c_ispeed;		/* input speed */
  //      speed_t c_ospeed;		/* output speed */
  //  #define _HAVE_STRUCT_TERMIOS_C_ISPEED 1
  //  #define _HAVE_STRUCT_TERMIOS_C_OSPEED 1
  //    };
  while(1 && g_keyboardinput!=113)
  {
    struct termios new_settings;
    struct termios stored_settings;
    tcgetattr(STDIN_FILENO,&stored_settings); //获得stdin 输入
    new_settings = stored_settings;           //
    new_settings.c_lflag &= (~ICANON);        //
    new_settings.c_cc[VTIME] = 0;
    tcgetattr(STDIN_FILENO,&stored_settings); //获得stdin 输入
    new_settings.c_cc[VMIN] = 1;
    tcsetattr(STDIN_FILENO,TCSANOW,&new_settings); //

    g_keyboardinput = getchar();

    tcsetattr(STDIN_FILENO,TCSANOW,&stored_settings);

    setbit(g_usrcmd, SETTING_ON);
    if(g_keyboardinput == 101)  //e
        setbit(g_usrcmd, SETTING_IMGENHANCE);
    else if(g_keyboardinput == 100) //d
        reversebit(g_usrcmd, SETTING_DETECTION);
    else if(g_keyboardinput == 99)  //c
        reversebit(g_usrcmd, SETTING_CROSS);
    else if(g_keyboardinput == 115)  //s
        reversebit(g_usrcmd, SETTING_SAVE);
    // return g_keyboardinput;
  }
}

int main(int argc, char *argv[])
{
    std::string yamlpath = "/home/nvidia/cfg/panocfg.yaml";
    if(argc > 1)
        yamlpath = argv[1];
    YAML::Node config = YAML::LoadFile(yamlpath);

    std::string loglvl = config["loglvl"].as<std::string>();
    if(loglvl == "critical-iair")
        spdlog::set_level(spdlog::level::critical);
    else if(loglvl == "trace-iair")
        spdlog::set_level(spdlog::level::trace);
    else if(loglvl == "warn-iair")
        spdlog::set_level(spdlog::level::warn);
    else if(loglvl == "info")
        spdlog::set_level(spdlog::level::info);
    else if(loglvl == "debug-iair")
        spdlog::set_level(spdlog::level::debug);
    else
        spdlog::set_level(spdlog::level::info);
    
    renderWidth = config["renderWidth"].as<int>();
    renderHeight = config["renderHeight"].as<int>();
    renderX = config["renderX"].as<int>();
    renderY = config["renderY"].as<int>();
    nvrenderCfg rendercfg{renderBufWidth, renderBufHeight, renderWidth, renderHeight, renderX, renderY, renderMode};
    pRenderer = new nvrender(rendercfg);

    cv::Mat screen = cv::Mat(renderBufHeight, renderBufWidth, CV_8UC3);
    screen.setTo(0);
    double fontScale = 1.2;
    int lineSickness = 2;
    int fontSickness = 2;
    cv::Scalar color = cv::Scalar(5, 217, 82 );
    std::string dispInitStr = "xjtu panorama app start";
    std::string dispFinalStr;// = "initialization"
            // cv::imshow("a",screen);
    // cv::waitKey(1);
    cv::Size testsz = cv::getTextSize(dispInitStr, cv::FONT_HERSHEY_SIMPLEX, fontScale, fontSickness, 0);
    cv::Point textpos((renderBufWidth - testsz.width)/2, (renderBufHeight - testsz.height)/2+200);

    cv::putText(screen, dispInitStr, textpos, cv::FONT_HERSHEY_SIMPLEX, fontScale, color, fontSickness);
    pRenderer->showImg(screen);

    panoAPP::context *appctx = new panoAPP::context(yamlpath);
    panoAPP::Factory::CreateState(appctx, panoAPP::PANOAPP_STATE_START);
    panoAPP::Factory::CreateState(appctx, panoAPP::PANOAPP_STATE_VERIFY);
    panoAPP::Factory::CreateState(appctx, panoAPP::PANOAPP_STATE_INIT);
    panoAPP::Factory::CreateState(appctx, panoAPP::PANOAPP_STATE_RUN);
    panoAPP::Factory::CreateState(appctx, panoAPP::PANOAPP_STATE_FINISH);

    appctx->start(panoAPP::PANOAPP_STATE_START);

    std::thread listerner = std::thread(scanKeyboard);
    listerner.detach();

    while(1 && g_keyboardinput!=113)
    {
        
        appctx->update();
        // spdlog::debug("g_keyboardinput:{}", g_keyboardinput);
    }
    if(pRenderer != nullptr)
        delete pRenderer;
    spdlog::info("close");

    return 0;

}