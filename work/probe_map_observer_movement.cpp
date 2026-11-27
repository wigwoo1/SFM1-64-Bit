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
#include "source_map_observer_movement_candidate.h"

void require(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
struct Query { int requested,previous,collide,oldCollide; unsigned flags; } query{};
struct State { uint8_t move,collide,prediction,simulation,dirty; uint32_t flags,contexts; } state{};
std::vector<unsigned> events;
uintptr_t actor=0;
constexpr uint32_t excluded=0x800000;
State initial() { return {uint8_t(query.previous),uint8_t(query.oldCollide),uint8_t(query.flags&64?3:0),
    uint8_t(query.flags&128?3:0),0xa0,uint32_t(0x1000|(query.flags&16?excluded:0)|(query.flags&32?4:0)),uint32_t(0x20|(query.flags&4?4:0))}; }
uint8_t enabled() { return query.flags&2?3:0; }
void physics() { if(query.flags&8) events.insert(events.end(),{1,2}); }
void context(bool v) { events.push_back(v?3:4); }
void movingList() { events.push_back(5); }

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T> T& at(uintptr_t p) { return *reinterpret_cast<T*>(p); }
uintptr_t physicsObject=0,contextObject=0;
void __fastcall physicsNotify(void* self,void*) { require(reinterpret_cast<uintptr_t>(self)==physicsObject,"physics owner"); events.push_back(2); }
int __fastcall physicsList(void* self,void*,uintptr_t* output,int maximum) {
    require(reinterpret_cast<uintptr_t>(self)==actor && maximum==1024,"physics list arguments");
    events.push_back(1); output[0]=0; output[1]=physicsObject; return 2;
}
void __fastcall addContext(void* self,void*,void* owner) {
    require(reinterpret_cast<uintptr_t>(self)==contextObject && reinterpret_cast<uintptr_t>(owner)==actor,"add context owner");
    require((at<uint32_t>(actor+0x36c)&4)!=0,"context bit set before registration"); context(true);
}
void __fastcall removeContext(void* self,void*,void* owner) {
    require(reinterpret_cast<uintptr_t>(self)==contextObject && reinterpret_cast<uintptr_t>(owner)==actor,"remove context owner");
    require((at<uint32_t>(actor+0x36c)&4)!=0,"context bit cleared too early"); context(false);
}
void boundary(unsigned slot,CONTEXT& c) {
    using O=PrivateX86BoundaryObserverFixture;
    if(slot==0) { O::returnCdecl(c,0); return; } // physics simulation predicate
    require(slot==1 && c.Ecx==0x105a0208 && at<uintptr_t>(c.Esp+4)==actor,"moving list arguments");
    require((at<uint32_t>(actor+0x11c)&excluded)==0,"moving-list exclusion not cleared");
    movingList(); O::returnCdecl(c); c.Esp+=4;
}
void run() {
    alignas(4) std::array<uint8_t,0x400> bytes{};
    std::array<uintptr_t,0x2a8/4> table{};
    std::array<uintptr_t,0x6c/4> physicsTable{};
    std::array<uintptr_t,4> contextTable{};
    uintptr_t physicsInstance=reinterpret_cast<uintptr_t>(physicsTable.data());
    uintptr_t contextInstance=reinterpret_cast<uintptr_t>(contextTable.data());
    actor=reinterpret_cast<uintptr_t>(bytes.data()); physicsObject=reinterpret_cast<uintptr_t>(&physicsInstance);
    contextObject=reinterpret_cast<uintptr_t>(&contextInstance);
    table[0x158/4]=at<uintptr_t>(0x1050eaac+0x158); // actual installed player predicate
    table[0x2a4/4]=reinterpret_cast<uintptr_t>(physicsList);
    physicsTable[0x68/4]=reinterpret_cast<uintptr_t>(physicsNotify);
    contextTable[2]=reinterpret_cast<uintptr_t>(addContext); contextTable[3]=reinterpret_cast<uintptr_t>(removeContext);
    at<uintptr_t>(actor)=reinterpret_cast<uintptr_t>(table.data());
    const auto s=initial();
    at<uint8_t>(actor+0x54)=uint8_t(query.flags&1); at<uint8_t>(actor+0x58)=s.dirty;
    at<uint32_t>(actor+0x11c)=s.flags; at<uint8_t>(actor+0x156)=s.move; at<uint8_t>(actor+0x157)=s.collide;
    at<uint8_t>(actor+0x2a5)=s.prediction; at<uint8_t>(actor+0x2a6)=s.simulation;
    at<uint32_t>(actor+0x36c)=s.contexts; at<uintptr_t>(actor+0x1d8)=query.flags&8?physicsObject:0;
    at<uint8_t>(0x105c14bc)=enabled(); at<uintptr_t>(0x1063cb0c)=contextObject;
    const auto before=bytes;
    reinterpret_cast<void (__thiscall*)(void*,int,int)>(0x100c2710)(bytes.data(),query.requested,query.collide);
    state={at<uint8_t>(actor+0x156),at<uint8_t>(actor+0x157),at<uint8_t>(actor+0x2a5),at<uint8_t>(actor+0x2a6),
        at<uint8_t>(actor+0x58),at<uint32_t>(actor+0x11c),at<uint32_t>(actor+0x36c)};
    auto expected=before; const auto p=reinterpret_cast<uintptr_t>(expected.data());
    at<uint8_t>(p+0x156)=state.move; at<uint8_t>(p+0x157)=state.collide; at<uint8_t>(p+0x2a5)=state.prediction;
    at<uint8_t>(p+0x2a6)=state.simulation; at<uint8_t>(p+0x58)=state.dirty;
    at<uint32_t>(p+0x11c)=state.flags; at<uint32_t>(p+0x36c)=state.contexts;
    require(bytes==expected,"unrelated actor bytes changed");
}
#else
struct Host {
    void dirty() { if(query.flags&1) state.dirty|=1; }
    int moveType() { return state.move; } uint8_t moveCollide() { return state.collide; }
    void setMoveType(uint8_t v) { dirty(); state.move=v; }
    void setMoveCollide(uint8_t v) { dirty(); state.collide=v; }
    void clearFlagFour() { state.flags&=~4u; }
    void physicsRulesChanged() { physics(); }
    uint8_t predictionByte() { return state.prediction; } uint8_t simulationByte() { return state.simulation; }
    void setPredictionByte(uint8_t v) { dirty(); state.prediction=v; }
    void setSimulationByte(uint8_t v) { dirty(); state.simulation=v; }
    uint8_t simulationEnabledByte() { return enabled(); }
    bool contextEnabled() { return bool(state.contexts&4); }
    void setContextEnabled(bool v) { if(v) state.contexts|=4; context(v); if(!v) state.contexts&=~4u; }
    bool excludedFromMovingList() { return bool(state.flags&excluded); }
    void clearMovingListExclusion() { state.flags&=~excluded; }
    void refreshMovingList() { movingList(); }
};
void run() { state=initial(); Host host; require(sourceMapSetObserverMovement(query.requested,query.collide,host),"valid request rejected"); }
#endif

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected installed server or -"); (void)argv;
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
        PrivateX86BoundaryObserverFixture observer{{0x10188b60,0x10110c70,0x10002800,0x10002600},boundary}; observer.configure(true);
#endif
        unsigned cases=0;
        for(int requested:{0,10}) for(int previous:{0,2,8,10}) for(int collide:{0,1,256})
        for(int oldCollide:{0,1}) for(unsigned flags=0;flags<256;++flags) {
            query={requested,previous,collide,oldCollide,flags}; auto expected=initial(); std::vector<unsigned> sequence;
            bool changed=false;
            if(previous==requested) { changed=expected.collide!=uint8_t(collide); expected.collide=uint8_t(collide); }
            else {
                changed=true; expected.move=uint8_t(requested); expected.collide=uint8_t(collide);
                if(previous==8) expected.flags&=~4u;
                expected.prediction=enabled(); expected.simulation=0;
                if(flags&8) sequence={1,2};
                if(bool(flags&2)!=bool(flags&4)) sequence.push_back(flags&2?3:4);
                expected.contexts=(expected.contexts&~4u)|(flags&2?4:0);
                if(flags&16) { expected.flags&=~excluded; sequence.push_back(5); }
            }
            if(changed && (flags&1)) expected.dirty|=1;
            events.clear(); run();
            require(state.move==expected.move && state.collide==expected.collide && state.prediction==expected.prediction &&
                state.simulation==expected.simulation && state.dirty==expected.dirty && state.flags==expected.flags &&
                state.contexts==expected.contexts && events==sequence,"observer movement mismatch"); ++cases;
        }
#ifndef _WIN64
        observer.configure(false); image.verify();
#else
        query={0,0,0,0,0}; state=initial(); Host host;
        require(!sourceMapSetObserverMovement(2,0,host),"unimplemented movement accepted");
#endif
        std::cout<<"{\"cases\":"<<cases<<",\"passed\":true}\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
