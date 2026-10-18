#include "graph_log_lookup_candidate.h"
#include <cassert>
#include <iostream>
struct Owner{int count=2;unsigned handles[2]={7,9};};struct Element{};
struct Adapter{Element element;unsigned reads=0,resolves=0,filters=0,filter=1;bool missing=false;std::uintptr_t symbol=2;
 int count68(Owner* p){return p->count;}unsigned handle5c(Owner* p,int i){++reads;return p->handles[i];}
 Element* resolveModelSlot18(unsigned h){++resolves;assert(h==7 || h==9);symbol=0x123456789;return missing?nullptr:&element;}Element* nullLog(){return nullptr;}
 std::uintptr_t liveSymbol10eb31d4(){return symbol;}unsigned filterElementSlot4(Element* e,std::uintptr_t s){++filters;assert(e==&element && s==0x123456789);return filter;}
};
int main(){static_assert(sizeof(void*)==8);Owner owner;Adapter a;assert(!graphLogLookupCandidate(&owner,-1,a) && !graphLogLookupCandidate(&owner,2,a) && !a.reads);assert(graphLogLookupCandidate(&owner,0,a)==&a.element);a.filter=256;assert(!graphLogLookupCandidate(&owner,1,a));a.missing=true;auto filters=a.filters;assert(!graphLogLookupCandidate(&owner,0,a) && a.filters==filters);owner.count=-1;auto reads=a.reads;assert(!graphLogLookupCandidate(&owner,0,a) && a.reads==reads);std::cout<<"x64 graph log lookup: signed bounds, handles, live wide symbol and low-byte type gate passed\n";}
