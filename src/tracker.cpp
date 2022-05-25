#include "tracker.h"
#include "spdlog/spdlog.h"
#include "stitcherglobal.h"

Tracker::Tracker()
{

}

Tracker::~Tracker()
{

}

int Tracker::init(std::vector<int> &detret)
{
    if(detret.size()%6)
    {
        spdlog::warn("detection result incomplete, tracker init failed");
        return RET_ERR;
    }

    return RET_OK;
}

int Tracker::update()
{

}