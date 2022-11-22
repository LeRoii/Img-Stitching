#include "panocam.h"
#include "panocamimpl.h"

panocam::panocam(std::string yamlpath):
    pimpl{std::make_unique<panocamimpl>(yamlpath)}
{
    // stNvrenderCfg rendercfg{1920, 1080, 1920/2, 1080/2, 0, 0};
    // pRenderer = new nvrender(rendercfg);

}

panocam::~panocam() = default;

int panocam::init()
{
    return pimpl->init();
}

int panocam::getCamFrame(int id, cv::Mat &frame)
{
    return pimpl->getCamFrame(id, frame);
}

int panocam::getPanoFrame(cv::Mat &ret)
{
    return pimpl->getPanoFrame(ret);
}

int panocam::detect(cv::Mat &img, std::vector<int> &ret)
{
    return pimpl->detect(img, ret);
}

int panocam::detect(cv::Mat &img)
{
    return pimpl->detect(img);
}

int panocam::imgEnhancement(cv::Mat &img)
{
    return pimpl->imgEnhancement(img);
}

int panocam::render(cv::Mat &img)
{
    return pimpl->render(img);
}

int panocam::drawCross(cv::Mat &img)
{
    return pimpl->drawCross(img);
}


int panocam::saveAndSend(cv::Mat &img)
{
    return pimpl->saveAndSend(img);
}

uint8_t panocam::getStatus()
{
    return pimpl->getStatus();
}