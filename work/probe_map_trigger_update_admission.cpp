#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>
#include "source_map_trigger_update_admission_candidate.h"

void require(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
struct Query { unsigned flags; bool type,edict,excluded,motion,engineFlag,alreadyMarked,caller; } query{};
std::vector<unsigned> events;
unsigned counter=0;
uint32_t actorFlags=0;
uintptr_t actor=0,edict=0,motion=0,engine=0;

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T> T& at(uintptr_t p) { return *reinterpret_cast<T*>(p); }
int __fastcall collisionMode(void* self,void*) {
    require(reinterpret_cast<uintptr_t>(self)==actor+0x164,"collision mode owner"); events.push_back(1); return 2;
}
void __fastcall solidUpdate(void* self,void*,void* entity,void* collision,void* previous,int engineValue) {
    require(reinterpret_cast<uintptr_t>(self)==engine && reinterpret_cast<uintptr_t>(entity)==edict &&
        reinterpret_cast<uintptr_t>(collision)==actor+0x164 && reinterpret_cast<uintptr_t>(previous)==motion &&
        engineValue==(query.engineFlag?3:0),"solid engine update arguments"); events.push_back(3);
}
void __fastcall triggerUpdate(void* self,void*,void* entity,int engineValue) {
    require(reinterpret_cast<uintptr_t>(self)==engine && reinterpret_cast<uintptr_t>(entity)==edict &&
        engineValue==(query.engineFlag?3:0),"trigger engine update arguments"); events.push_back(4);
}
void boundary(unsigned slot,CONTEXT& c) {
    require(slot==0 && at<uintptr_t>(c.Esp+4)==actor,"update-list notification arguments");
    require(at<unsigned>(actor+0x84)==18 && (at<uint32_t>(actor+0x11c)&0x1000000),"update notification before fields");
    events.push_back(2); PrivateX86BoundaryObserverFixture::returnCdecl(c);
}
void run() {
    alignas(4) std::array<uint8_t,0x240> bytes{};
    std::array<uint8_t,0x20> helper{};
    std::array<uintptr_t,12> collisionTable{};
    std::array<uintptr_t,0x134/4> engineTable{};
    uintptr_t engineObject=reinterpret_cast<uintptr_t>(engineTable.data());
    int edictObject=0,motionObject=0;
    actor=reinterpret_cast<uintptr_t>(bytes.data()); edict=query.edict?reinterpret_cast<uintptr_t>(&edictObject):0;
    motion=query.motion?reinterpret_cast<uintptr_t>(&motionObject):0; engine=reinterpret_cast<uintptr_t>(&engineObject);
    collisionTable[0x2c/4]=reinterpret_cast<uintptr_t>(collisionMode);
    engineTable[0x12c/4]=reinterpret_cast<uintptr_t>(solidUpdate); engineTable[0x130/4]=reinterpret_cast<uintptr_t>(triggerUpdate);
    at<uintptr_t>(actor+0x164)=reinterpret_cast<uintptr_t>(collisionTable.data());
    at<uintptr_t>(actor+0x18)=edict; at<uint8_t>(actor+0x184)=uint8_t(query.flags);
    at<uint8_t>(actor+0x186)=query.type?2:0;
    at<unsigned>(actor+0x84)=17; at<uint32_t>(actor+0x11c)=0x2000|(query.alreadyMarked?0x1000000:0);
    at<uintptr_t>(0x106716d4)=query.excluded?actor:0;
    at<uintptr_t>(0x10635ca8)=engine; at<uint8_t>(0x105938fc)=query.engineFlag?3:0;
    const auto before=bytes;
    if(query.caller) {
        require((query.flags&4) && !query.motion,"caller requires early non-solid path and null motion");
        at<uintptr_t>(reinterpret_cast<uintptr_t>(helper.data())+4)=actor;
        // Actual CMoveHelperServer impact routine. Non-solid flag returns
        // immediately after the actual trigger-update admission call.
        reinterpret_cast<void (__thiscall*)(void*)>(0x101776a0)(helper.data());
    } else reinterpret_cast<void (__thiscall*)(void*,void*)>(0x100cddb0)(bytes.data(),reinterpret_cast<void*>(motion));
    counter=at<unsigned>(actor+0x84); actorFlags=at<uint32_t>(actor+0x11c);
    auto expected=before; const auto p=reinterpret_cast<uintptr_t>(expected.data());
    at<unsigned>(p+0x84)=counter; at<uint32_t>(p+0x11c)=actorFlags;
    require(bytes==expected,"unrelated actor storage changed");
}
#else
struct Host {
    bool hasEdict() { return query.edict; } bool isExcludedActor() { return query.excluded; }
    unsigned solidFlags() { return query.flags; } int solidType() { return query.type?2:0; }
    void collisionMode() { events.push_back(1); }
    void markForUpdate() { ++counter; if(!(actorFlags&0x1000000)) { actorFlags|=0x1000000; events.push_back(2); } }
    void updateSolid() { events.push_back(3); } void updateTrigger() { events.push_back(4); }
};
void run() { counter=17; actorFlags=0x2000|(query.alreadyMarked?0x1000000:0); Host host; sourceMapAdmitTriggerUpdateCandidate(host); }
#endif

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected installed server or -"); (void)argv;
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
        PrivateX86BoundaryObserverFixture observer{{0x10111000,0x10002800,0x10002600,0x10002400},boundary}; observer.configure(true);
#endif
        unsigned cases=0,callerCases=0;
        for(unsigned flags=0;flags<16;++flags) for(unsigned bits=0;bits<64;++bits) {
            query={flags,bool(bits&1),bool(bits&2),bool(bits&4),bool(bits&8),bool(bits&16),bool(bits&32),false};
            for(int route=0;route<2;++route) {
                if(route && (!(flags&4) || query.motion)) continue;
                query.caller=route!=0;
                const bool admitted=query.edict && !query.excluded && ((flags&8) || (query.type && !(flags&4)));
                std::vector<unsigned> expected;
                if(admitted) { expected.push_back(1); if(!query.alreadyMarked) expected.push_back(2); expected.push_back(flags&8?4:3); }
                events.clear(); run();
                require(events==expected && counter==unsigned(admitted?18:17) &&
                    actorFlags==uint32_t(0x2000|((admitted || query.alreadyMarked)?0x1000000:0)),"trigger update admission mismatch");
                if(route) ++callerCases; else ++cases;
            }
        }
#ifndef _WIN64
        observer.configure(false); image.verify();
#endif
        std::cout<<"{\"cases\":"<<cases<<",\"caller_cases\":"<<callerCases<<",\"passed\":true}\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
