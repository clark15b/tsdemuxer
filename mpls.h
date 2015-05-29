#ifndef __MPLS_H
#define __MPLS_H

#include "common.h"

namespace mpls
{
    int parse(const char* filename,std::list<int>& playlist,std::map<int,std::string>& datetime,int verb);
}

#endif
