#include "attribute_initialize_candidate.h"
#include <cassert>
#include <iostream>
struct Owner{};
struct Attribute{void* data;Owner* owner;std::uintptr_t symbol;std::uint32_t handle;std::uint16_t flags;Attribute* next;};
struct Adapter {
    Attribute* current;unsigned seenType=0;
    void setData(Attribute* p,void* v){p->data=v;}
    void setNameSymbol(Attribute* p,std::uintptr_t v){p->symbol=v;}
    void setOwner(Attribute* p,Owner* v){p->owner=v;}
    std::uintptr_t internName(const char*){assert(current->symbol==~std::uintptr_t{0});return 0x100000007ull;}
    void setFlags(Attribute* p,std::uint16_t v){p->flags=v;}
    auto flags(Attribute* p){return p->flags;}
    auto data(Attribute* p){return p->data;}
    void setHandle(Attribute* p,std::uint32_t v){p->handle=v;}
    void setNext(Attribute* p,Attribute* v){p->next=v;}
    void initializeTypedData(unsigned t,void* v){seenType=t;assert(v==current->data && !(current->flags&0x200));assert(current->symbol==0x100000007ull && current->next==nullptr && current->handle==0xffffffff);}
};
int main(){Owner owner;int data=10;
    for(auto type:{1u,5u,15u,33u}){Attribute p{};Adapter a{&p};assert(initializeAttributeCandidate(&p,&owner,type,"name",&data,a)==&p);assert(p.owner==&owner && p.data==&data && a.seenType==(type&31));assert(p.flags==(type|0x200|((type==1||type==15)?0x400:0)));}
    std::cout<<"x64 attribute initialization: field/order/exact type flag/low5 dispatch passed\n";
}
