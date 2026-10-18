#include "controller_window_title_candidate.h"
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
struct Child{};
struct Adapter {
    Child child;bool hasWindow=true,hasChild=true;std::vector<int> events;std::string title;unsigned modified=0;
    void* window(){return hasWindow?this:nullptr;}Child* globalChild(){return hasChild?&child:nullptr;}
    unsigned char childModifiedByte(Child*){return 128;}
    void setWindowModified(void*,unsigned char v){modified=v;events.push_back(1);}
    const char* childTitleSource(Child*){events.push_back(2);return "session";}
    std::string fromAscii(const char* p){events.push_back(3);return p;}
    std::string translate(const char* p){events.push_back(4);return p;}
    std::string arg(std::string s,const std::string& value,int width,char fill){assert(width==0 && fill==' ');events.push_back(5);auto pos=s.find("%1");if(pos==std::string::npos)pos=s.find("%2");s.replace(pos,2,value);return s;}
    void setWindowTitle(void*,const std::string& v){title=v;events.push_back(6);}
};
int main(){Adapter a;updateControllerWindowTitleCandidate(a);assert(a.modified==128 && a.title=="session[*] - Source Filmmaker [Beta]");assert((a.events==std::vector<int>{1,2,3,4,4,5,5,6}));
    Adapter b;b.hasChild=false;updateControllerWindowTitleCandidate(b);assert(b.modified==0 && b.title=="Source Filmmaker [Beta]" && (b.events==std::vector<int>{1,4,6}));
    Adapter c;c.hasWindow=false;updateControllerWindowTitleCandidate(c);assert(c.events.empty());
    std::cout<<"x64 controller title protocol: missing window/absent child/raw modified byte/translation order/Qt marker preserved passed\n";
}
