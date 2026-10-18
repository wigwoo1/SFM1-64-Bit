#include "graph_element_refresh_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Adapter{int one=1,two=2,element=3,log=4;int* transient=&one;bool missing=false;unsigned recording=128;std::vector<int> events;
 void beginProfile(){events.push_back(1);}int* transient4e8(){return transient;}unsigned handle5c(){return 7;}int* resolveModelSlot18(unsigned h){assert(h==7);return missing?nullptr:&element;}
 unsigned recordingModelSlot61(){return recording;}void setRecordingModelSlot60(unsigned r){recording=r;events.push_back(r?3:2);}
 void transientSlot38(int* p,unsigned bits,unsigned zero){assert(p==&one && bits==0x3a83126f && !zero && !recording);transient=&two;events.push_back(4);}
 int* getLog10516a30(int* p,unsigned z){assert(p==&element && !z && recording==128);events.push_back(5);return &log;}
 void logSlot14(int* l,int* p){assert(l==&log && p==&two);recording=7;events.push_back(6);}void elementSlot56(int* p){assert(p==&element && !recording);events.push_back(7);}
 void clearTransient4e8(){transient=nullptr;events.push_back(8);}void afterTransient1073f430(unsigned z){assert(!z && !transient);events.push_back(9);}
 void finalRefresh107358e0(){events.push_back(10);}void endProfile(){events.push_back(11);}
};
int main(){Adapter a;graphElementRefreshCandidate(a);assert((a.events==std::vector<int>{1,2,4,3,5,6,2,7,3,8,9,10,11}) && a.recording==7);Adapter b;b.missing=true;graphElementRefreshCandidate(b);assert((b.events==std::vector<int>{1,8,9,10,11}));Adapter c;c.transient=nullptr;graphElementRefreshCandidate(c);assert((c.events==std::vector<int>{1,10,11}));std::cout<<"x64 graph refresh: live transient reload, separate recording scopes, missing handle and clear/update order passed\n";}
