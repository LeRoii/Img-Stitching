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

    int objnum = detret.size()/6;
    for(int i=0;i<objnum;i++)
    {
        Object obj;
        obj.x = detret[6*i];
        obj.y = detret[6*i+1];
        obj.w = detret[6*i+2];
        obj.h = detret[6*i+3];
        obj.label = detret[6*i+4];
        obj.prob = detret[6*i+5];
        m_stTrackerList.push_back(obj);
    }

    return RET_OK;
}

int Tracker::update(std::vector<int> &detret)
{
    if(detret.size()%6)
    {
        spdlog::warn("detection result incomplete, tracker init failed");
        return RET_ERR;
    }

    int objnum = detret.size()/6;
    m_stDetList.clear();
    for(int i=0;i<objnum;i++)
    {
        Object obj;
        obj.x = detret[6*i];
        obj.y = detret[6*i+1];
        obj.w = detret[6*i+2];
        obj.h = detret[6*i+3];
        obj.label = detret[6*i+4];
        obj.prob = detret[6*i+5];
        m_stDetList.push_back(obj);
    }
}

int Tracker::calDistMat()
{
    int detNum = m_stDetList.size();
    int trackNum = m_stTrackerList.size();
    memset(m_distMat, 0, sizeof(m_distMat));
    for(int i=0;i<detNum;i++)
    {
        for(int j=0;j<trackNum;j++)
        {
            m_distMat[i][j] = calDist(m_stDetList[i], m_stTrackerList[j]);
        }
    }

}

bool Tracker::match(int detObjIdx)
{
    for(int i=0;i<m_stTrackerList.size();i++)
    {
        if(m_distMat[detObjIdx][i] < DIST_THRESH && !m_visited[i])
        {
            m_visited[i] = true;
            if(m_matchedDetObjIdx[i] == -1 || match(m_matchedDetObjIdx[i]))
            {
                m_matchedDetObjIdx[i] = detObjIdx;
                return true;
            }
        }
    }
    return false;
}

int Tracker::hgrMatch()
{
    int cnt = 0;
    for(int i=0;i<m_stDetList.size();i++)
    {
        memset(m_visited, 0, sizeof(m_visited));
        if(match(i))    //match succedd
        {
            cnt++;
        }    
        else    //match failed
        {

        }
    }

    return cnt;
}