#ifndef _TRACKER_H_
#define _TRACKER_H_

#include <list>
#include <vector>

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
    int update();
private:
    std::list<Object> m_stTrackerlist;
};

#endif