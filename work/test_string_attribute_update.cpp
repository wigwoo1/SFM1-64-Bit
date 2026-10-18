#include "string_attribute_update_candidate.h"
#include <cassert>
#include <vector>
#include <iostream>
struct Attribute{std::uint16_t flags=5;std::uintptr_t symbol=7;};
struct Adapter {
    int phaseValue=0;bool record=false,mutateRecord=false,notify=true;
    std::vector<unsigned> events;unsigned propagated=0,modelMask=0;
    auto flags(Attribute* p){return p->flags;}
    int phase(){events.push_back(2);return phaseValue;}
    void prepareCallbacks(Attribute*){events.push_back(3);}
    void setFlags(Attribute* p,std::uint16_t v){p->flags=v;events.push_back(4);}
    void markOwner(Attribute*){events.push_back(5);}
    void initializeStringCompareDefault(){events.push_back(1);}
    std::uintptr_t stringCompareDefault(){return ~std::uintptr_t{0};}
    auto stringSymbol(Attribute* p){return p->symbol;}
    bool shouldRecordChange(Attribute*){events.push_back(6);return record;}
    void* createStringChangeRecord(Attribute* p,const std::uintptr_t* source){events.push_back(7);assert(p->symbol==7);if(mutateRecord)*const_cast<std::uintptr_t*>(source)=0x10000000bull;return nullptr;}
    void submitStringChangeRecord(void* recordPointer){assert(!recordPointer);events.push_back(8);}
    void setStringSymbol(Attribute* p,std::uintptr_t v){events.push_back(9);p->symbol=v;}
    bool ownerNotificationReady(Attribute*){events.push_back(10);return notify;}
    void notifyOwner(Attribute* p){assert(p->symbol!=7);events.push_back(11);}
    void propagateOwner(Attribute*,unsigned mask){propagated=mask;events.push_back(12);}
    void markModelChange(unsigned mask){modelMask|=mask;events.push_back(13);}
};
int main(){
    Attribute p;Adapter a;std::uintptr_t source=7;
    updateStringAttributeCandidate(&p,&source,a);assert((a.events==std::vector<unsigned>{1}) && p.flags==5);
    a.events.clear();source=9;a.phaseValue=3;
    updateStringAttributeCandidate(&p,&source,a);assert(p.symbol==7 && (a.events==std::vector<unsigned>{1,2}));
    a.events.clear();a.phaseValue=0;p.flags=0x2105;a.record=true;a.mutateRecord=true;
    updateStringAttributeCandidate(&p,&source,a);
    assert(p.symbol==0x10000000bull && a.propagated==2 && a.modelMask==0x2000);
    assert((a.events==std::vector<unsigned>{1,2,3,4,5,6,7,8,9,10,11,12,13}));
    a.events.clear();updateStringAttributeCandidate(&p,&source,a);assert((a.events==std::vector<unsigned>{1}));
    a.events.clear();p.flags=2;source=12;updateStringAttributeCandidate(&p,&source,a);assert(a.events.empty());
    std::cout<<"x64 string update: unchanged/denied/live source/record-before-write/notification/content propagation passed\n";
}
