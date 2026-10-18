#include "graph_view_dispatch_candidate.h"
#include <cassert>
#include <iostream>
struct Adapter{int inherited=0,context=0,escape=0,keys=0;void* received=nullptr;
 int inheritedMetacall(int,int,void**){return inherited;}
 void contextMenuSlot50(void* p){++context;received=p;}void escapeSlot51(){++escape;}
 void keyEvent100d3ef0(void* p){++keys;received=p;}
};
int main(){static_assert(sizeof(void*)==8);Adapter a;int point[2]={12,-9};void* event=reinterpret_cast<void*>(std::uintptr_t{0x123456789});void* argv[]={nullptr,point};
 assert(graphViewMetacallCandidate(0,0,argv,a)==-4 && a.context==1 && a.received==point);
 a.inherited=1;assert(graphViewMetacallCandidate(0,0,nullptr,a)==-3 && a.escape==1);
 argv[1]=&event;for(int i=2;i<4;++i){a.inherited=i;assert(graphViewMetacallCandidate(0,0,argv,a)==i-4 && a.received==event);}assert(a.keys==2);
 a.inherited=4;assert(graphViewMetacallCandidate(0,0,nullptr,a)==0);a.inherited=-1;assert(graphViewMetacallCandidate(0,0,nullptr,a)==-1);a.inherited=3;assert(graphViewMetacallCandidate(1,0,nullptr,a)==3);
 graphViewStaticDispatchCandidate(0,0xffffffff,nullptr,a);assert(a.context==1 && a.escape==1 && a.keys==2);
 std::cout<<"x64 graph view dispatch: context reference, Escape, shared key handler, wide event pointer and inherited ID gates passed\n";
}
