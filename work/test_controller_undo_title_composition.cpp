#include "controller_undo_listener_candidate.h"
#include "controller_window_title_candidate.h"
#include <cassert>
#include <string>
#include <vector>
#include <iostream>
struct Child{unsigned char state=0;};
struct Adapter {
    Child child;bool hasWindow=true,hasChild=true;std::vector<int> events;std::string title;unsigned modified=0;unsigned char flag=0;std::uint64_t timestamp=0;
    Child* globalChild344(){return globalChild();}
    auto childByte15(Child* c){return c->state;}
    void setChildByte15(Child* c,unsigned char v){c->state=v;}
    void invokeGlobalControllerChange(std::uint32_t){updateControllerWindowTitleCandidate(*this);}
    void setListenerByte219(unsigned char v){flag=v;}
    std::uint64_t readTimestampCounter(){assert(flag==1);return 0x123456789abcdef0ull;}
    void setListenerTimestamp228(std::uint64_t v){timestamp=v;}
    void* window(){return hasWindow?this:nullptr;}Child* globalChild(){return hasChild?&child:nullptr;}
    unsigned char childModifiedByte(Child*){return child.state;}
    void setWindowModified(void*,unsigned char v){modified=v;events.push_back(1);}
    const char* childTitleSource(Child*){events.push_back(2);return "session";}
    std::string fromAscii(const char* p){events.push_back(3);return p;}
    std::string translate(const char* p){events.push_back(4);return p;}
    std::string arg(std::string s,const std::string& value,int width,char fill){assert(width==0 && fill==' ');events.push_back(5);auto pos=s.find("%1");if(pos==std::string::npos)pos=s.find("%2");s.replace(pos,2,value);return s;}
    void setWindowTitle(void*,const std::string& v){title=v;events.push_back(6);}
};
int main(){
    Adapter a;controllerUndoListenerCandidate(a);
    assert(a.modified==1 && a.title=="session[*] - Source Filmmaker [Beta]" && a.flag==1 && a.timestamp==0x123456789abcdef0ull);
    assert((a.events==std::vector<int>{1,2,3,4,4,5,5,6}));
    a.events.clear();controllerUndoListenerCandidate(a);assert(a.events.empty() && a.flag==1);
    Adapter b;b.hasWindow=false;controllerUndoListenerCandidate(b);assert(b.child.state==1 && b.events.empty() && b.flag==1);
    Adapter c;c.hasChild=false;controllerUndoListenerCandidate(c);assert(c.events.empty() && c.flag==1 && c.timestamp);
    std::cout<<"x64 controller undo-to-title composition: changed updates title/unchanged suppression/missing window or child/timestamp passed\n";
}
