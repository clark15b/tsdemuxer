#ifndef __MPLS_H
#define __MPLS_H

#include <map>
#include <string>
#include <list>

int mpls_parse(const char* filename,std::list<int>& playlist,std::map<int,std::string>& datetime,int verb);

#endif
