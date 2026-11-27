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
#include "source_map_empty_player_tick_candidate.h"

void require(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
struct Query { int tick,previous,parent; bool parentMarksTick,special; } query{};
int lastTick=0;
std::vector<unsigned> events;
uintptr_t player=0,parent=0;

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T> T& at(uintptr_t p) { return *reinterpret_cast<T*>(p); }
void __fastcall enterProfile(void*,void*,const char* name,int detail,const char*,bool,int budget) {
    require(std::strcmp(name,"CBasePlayer::PhysicsSimulate")==0 && detail==0 && budget==4,"profile entry arguments"); events.push_back(1);
}
void __fastcall exitProfile(void*,void*) { events.push_back(9); }
void __fastcall simulateParent(void* owner,void*) {
    require(reinterpret_cast<uintptr_t>(owner)==parent,"parent owner"); events.push_back(2);
    if(query.parentMarksTick) at<int>(player+0xfc)=query.tick;
}
void boundary(unsigned slot,CONTEXT& c) {
    using O=PrivateX86BoundaryObserverFixture;
    if(slot==0) {
        require(c.Ecx==player,"special owner"); events.push_back(5); O::returnCdecl(c);
    } else if(slot==1 || slot==2) {
        require(c.Ecx==player+(slot==1?0:0xce4),"count/cleanup owner");
        events.push_back(slot==1?3:6);
        // Execute-through observation: original callee runs. Resume flag
        // suppresses this hardware breakpoint for its first instruction only.
        c.EFlags|=0x10000;
    } else throw std::runtime_error("unexpected command processing");
}
void run() {
    alignas(4) std::array<uint8_t,0x1300> bytes{};
    std::array<uintptr_t,0x1b4/4> table{};
    std::array<uint8_t,0x30> handles{},globals{};
    uintptr_t parentObject=reinterpret_cast<uintptr_t>(table.data());
    player=reinterpret_cast<uintptr_t>(bytes.data()); parent=reinterpret_cast<uintptr_t>(&parentObject);
    table[0x1b0/4]=reinterpret_cast<uintptr_t>(simulateParent);
    at<int>(player+0xfc)=query.previous; at<uint8_t>(player+0xbc8)=query.special;
    at<uint32_t>(player+0x158)=query.parent==0?UINT32_MAX:0x20001;
    at<uintptr_t>(reinterpret_cast<uintptr_t>(handles.data())+20)=query.parent==3?0:parent;
    at<uint32_t>(reinterpret_cast<uintptr_t>(handles.data())+24)=query.parent==2?3:2;
    at<uintptr_t>(0x105a01e0)=reinterpret_cast<uintptr_t>(handles.data());
    const auto g=reinterpret_cast<uintptr_t>(globals.data());
    at<float>(g+0xc)=1.25f; at<float>(g+0x10)=0.03125f; at<float>(g+0x1c)=0.03125f;
    at<int>(g+0x14)=2; at<int>(g+0x18)=query.tick; at<int>(g+0x24)=1;
    at<uintptr_t>(0x10635ca0)=g;
    at<uintptr_t>(0x1043c250)=reinterpret_cast<uintptr_t>(enterProfile);
    at<uintptr_t>(0x1043c248)=reinterpret_cast<uintptr_t>(exitProfile);
    const auto before=bytes; const auto beforeGlobals=globals;
    const auto entry=at<uintptr_t>(0x1050eaac+0x1b0);
    for(int repeat=0;repeat<2;++repeat) reinterpret_cast<void (__thiscall*)(void*)>(entry)(bytes.data());
    lastTick=at<int>(player+0xfc);
    auto expected=before; at<int>(reinterpret_cast<uintptr_t>(expected.data())+0xfc)=lastTick;
    require(bytes==expected,"unexpected player storage change");
    require(globals==beforeGlobals,"empty player simulation changed server globals");
}
#else
struct Host {
    void enterProfile() { events.push_back(1); } void exitProfile() { events.push_back(9); }
    bool hasValidParent() { return query.parent==1; }
    void simulateParent() { events.push_back(2); if(query.parentMarksTick) ::lastTick=query.tick; }
    int lastTick() { return ::lastTick; } int serverTick() { return query.tick; }
    void setLastTick(int value) { ::lastTick=value; }
    void countQueuedTicks() { events.push_back(3); }
    bool specialProcessing() { return query.special; }
    void processSpecial() { events.push_back(5); }
    void clearEmptyCommandContexts() { events.push_back(6); }
};
void run() { lastTick=query.previous; Host host; for(int i=0;i<2;++i) sourceMapSimulateEmptyPlayerTickCandidate(host); }
#endif

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected installed server or -"); (void)argv;
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
        PrivateX86BoundaryObserverFixture observer{{0x101c5650,0x101bed20,0x101cd2d0,0x101775f0},boundary}; observer.configure(true);
#endif
        unsigned cases=0;
        for(int tick:{0,3}) for(int previous:{-1,0,3}) for(int parentCase:{0,1,2,3})
        for(bool mark:{false,true}) for(bool special:{false,true}) {
            query={tick,previous,parentCase,mark,special};
            std::vector<unsigned> expected; int expectedTick=previous;
            for(int repeat=0;repeat<2;++repeat) {
                expected.push_back(1);
                if(parentCase==1) { expected.push_back(2); if(mark) expectedTick=tick; }
                if(expectedTick!=tick) {
                    expectedTick=tick; expected.push_back(3); if(special) expected.push_back(5); expected.push_back(6);
                }
                expected.push_back(9);
            }
            events.clear(); run();
            require(lastTick==expectedTick && events==expected,"empty player tick mismatch"); ++cases;
        }
#ifndef _WIN64
        observer.configure(false); image.verify();
#endif
        std::cout<<"{\"cases\":"<<cases<<",\"passed\":true}\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
