#include "attribute_write_gate_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Attribute{std::uint16_t flags=0;};
struct Adapter {
    int currentPhase=0;bool mutate=false;std::vector<unsigned> events;
    auto flags(Attribute* p){return p->flags;}
    int phase(){events.push_back(1);return currentPhase;}
    void prepareCallbacks(Attribute* p){events.push_back(2);if(mutate)p->flags|=0x1000;}
    void setFlags(Attribute* p,std::uint16_t f){events.push_back(3);p->flags=f;}
    void markOwner(Attribute* p){events.push_back(4);assert((p->flags&0x4080)==0x4080);}
};
int main(){
    unsigned cases=0;
    for(int phase=-1;phase<8;++phase)for(unsigned flags=0;flags<65536;++flags){
        Attribute p{static_cast<std::uint16_t>(flags)};Adapter a;a.currentPhase=phase;
        const bool allowed=!(flags&0x20) && (phase==0||phase==1||phase==2||(phase==4 && !(flags&0x400)));
        assert(attributeWriteGateCandidate(&p,a)==allowed);
        assert(p.flags==(allowed?static_cast<std::uint16_t>(flags|0x4080):flags));
        if(flags&0x20)assert(a.events.empty());
        ++cases;
    }
    Attribute p{0x2000};Adapter a;a.mutate=true;
    assert(attributeWriteGateCandidate(&p,a) && p.flags==0x7080 && (a.events==std::vector<unsigned>{1,2,3,4}));
    a.events.clear();assert(attributeWriteGateCandidate(&p,a) && (a.events==std::vector<unsigned>{1,3,4}));
    std::cout<<"x64 write gate: "<<cases<<" phase/flag cases; live mutation/prepare once/order passed\n";
}
