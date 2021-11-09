#ifndef _PANOCAM_H_
#define _PANOCAM_H_

#include <memory>

class __attribute__((visibility("default"))) panocam
{
public:
    panocam();
    ~panocam();

private:
    class panocamimpl;
    std::unique_ptr<panocamimpl> pimpl;

};

#endif