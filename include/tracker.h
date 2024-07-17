#ifndef _TRACKER_H_
#define _TRACKER_H_

#include <list>
#include <vector>

const int MAX_DET_NUM = 100;
const int MAX_TRACK_NUM = 100;
const float DIST_THRESH = 100;
struct Object
{
    int label;
    int x;
    int y;
    int w;
    int h;
    int prob;
    int id;
};

class Tracker
{
public:
    Tracker();
    ~Tracker();
    int init(std::vector<int> &detret);
    int update(std::vector<int> &detret);
private:
    float calDist(const Object obj1, const Object obj2);
    int calDistMat();
    bool match(int detObjIdx);
    int hrgMatch();
    std::vector<Object> m_stTrackerList;
    std::vector<Object> m_stDetList;
    float m_distMat[MAX_DET_NUM][MAX_TRACK_NUM];
    int m_matchedDetObjIdx[MAX_TRACK_NUM];
    bool m_visited[MAX_TRACK_NUM];
};

#endif