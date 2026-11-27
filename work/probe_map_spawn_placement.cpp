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
#include "source_map_spawn_placement_candidate.h"

void require(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
using Vec=std::array<float,3>;
struct Bounds { Vec view,minimum,maximum; } hull{{0,0,64},{-24,-24,0},{24,24,82}};
Vec spawnOrigin{},spawnAngles{},traceEnd{},position{},localAngles{},eyeAngles{},vectorA{},vectorB{};
std::vector<unsigned> events;
uintptr_t actor=0,selected=0,rules=0;
Vec trace(const Vec& start,const Vec& end,const Vec& minimum,const Vec& maximum) {
    auto expected=spawnOrigin; expected[2]-=500.f;
    require(start==spawnOrigin && end==expected && minimum==hull.minimum && maximum==hull.maximum,"spawn trace geometry");
    events.push_back(3); return traceEnd;
}

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T> T& at(uintptr_t p) { return *reinterpret_cast<T*>(p); }
PrivateX86BoundaryObserverFixture* activeObserver=nullptr;
void* __fastcall selectSpawn(void* self,void*) { require(reinterpret_cast<uintptr_t>(self)==actor,"spawn selector owner"); events.push_back(1); return reinterpret_cast<void*>(selected); }
const Bounds* __fastcall bounds(void* self,void*) { require(reinterpret_cast<uintptr_t>(self)==rules,"spawn bounds owner"); events.push_back(2); return &hull; }
void __fastcall changed(void* self,void*,void* field) {
    require(reinterpret_cast<uintptr_t>(self)==actor+0x938,"reset vector owner");
    const auto address=reinterpret_cast<uintptr_t>(field);
    require(address==actor+0x9a8 || address==actor+0x9b4,"reset vector field"); events.push_back(address==actor+0x9a8?7:8);
}
Vec readVector(uintptr_t address) { Vec result; std::memcpy(result.data(),reinterpret_cast<const void*>(address),12); return result; }
void boundary(unsigned slot,CONTEXT& c) {
    using O=PrivateX86BoundaryObserverFixture;
    if(slot==0 && c.Eip==0x10019790) {
        const auto a=c.Esp;
        require(at<uint32_t>(a+20)==0x200400b && at<uintptr_t>(a+24)==actor && at<int>(a+28)==0,"spawn trace mask/ignore/group");
        const auto hit=trace(readVector(at<uintptr_t>(a+4)),readVector(at<uintptr_t>(a+8)),
            readVector(at<uintptr_t>(a+12)),readVector(at<uintptr_t>(a+16)));
        std::memcpy(reinterpret_cast<void*>(at<uintptr_t>(a+32)+0xc),hit.data(),12);
        // Reuse the trace execution breakpoint after it returns for the final
        // eye-angle setter. No original executable bytes are changed.
        activeObserver->entries[0]=0x101b9950; c.Dr0=0x101b9950; c.ContextFlags|=CONTEXT_DEBUG_REGISTERS;
        O::returnCdecl(c); return;
    }
    require(c.Ecx==actor,"spawn transform owner");
    const auto value=readVector(at<uintptr_t>(c.Esp+4));
    if(slot==1) { position=value; events.push_back(4); }
    else if(slot==2) { require(value==Vec{},"spawn velocity request"); events.push_back(5); }
    else if(slot==3) { localAngles=value; events.push_back(6); }
    else { require(c.Eip==0x101b9950,"spawn final view boundary"); eyeAngles=value; events.push_back(9); }
    O::returnCdecl(c); c.Esp+=4;
}
void run() {
    alignas(4) std::array<uint8_t,0x1100> bytes{};
    alignas(4) std::array<uint8_t,0x360> spawn{};
    std::array<uintptr_t,0x690/4> table{};
    std::array<uintptr_t,0x7c/4> ruleTable{};
    const uintptr_t changedTable[]={reinterpret_cast<uintptr_t>(changed)};
    uintptr_t ruleObject=reinterpret_cast<uintptr_t>(ruleTable.data());
    actor=reinterpret_cast<uintptr_t>(bytes.data()); selected=reinterpret_cast<uintptr_t>(spawn.data()); rules=reinterpret_cast<uintptr_t>(&ruleObject);
    table[0x68c/4]=reinterpret_cast<uintptr_t>(selectSpawn); ruleTable[0x78/4]=reinterpret_cast<uintptr_t>(bounds);
    at<uintptr_t>(actor)=reinterpret_cast<uintptr_t>(table.data()); at<uintptr_t>(actor+0x938)=reinterpret_cast<uintptr_t>(changedTable);
    std::memcpy(spawn.data()+0x280,spawnOrigin.data(),12); std::memcpy(spawn.data()+0x348,spawnAngles.data(),12);
    std::memcpy(bytes.data()+0x9a8,vectorA.data(),12); std::memcpy(bytes.data()+0x9b4,vectorB.data(),12);
    at<uintptr_t>(0x10636664)=rules;
    std::memset(reinterpret_cast<void*>(0x106de6ac),0,24);
    const auto before=bytes;
    const auto beforeSpawn=spawn;
    const auto returned=reinterpret_cast<void* (__thiscall*)(void*,void*)>(0x102fb050)(reinterpret_cast<void*>(rules),bytes.data());
    require(reinterpret_cast<uintptr_t>(returned)==selected,"placement result entity");
    vectorA=readVector(actor+0x9a8); vectorB=readVector(actor+0x9b4);
    auto expected=before; std::memset(expected.data()+0x9a8,0,24);
    require(bytes==expected && spawn==beforeSpawn,"placement changed unrelated player/selected entity storage");
}
#else
struct Host {
    uintptr_t selectSpawn() { events.push_back(1); return 1; }
    const Bounds& bounds() { events.push_back(2); return hull; }
    Vec spawnOrigin(uintptr_t) { return ::spawnOrigin; }
    Vec spawnAngles(uintptr_t) { return ::spawnAngles; }
    Vec trace(const Vec& a,const Vec& b,const Vec& c,const Vec& d,unsigned mask,int group) {
        require(mask==0x200400b && group==0,"owned trace mask/group"); return ::trace(a,b,c,d);
    }
    void setLocalOrigin(const Vec& value) { position=value; events.push_back(4); }
    void zeroVelocity() { events.push_back(5); }
    void setLocalAngles(const Vec& value) { localAngles=value; events.push_back(6); }
    void resetVectorA() { if(vectorA!=Vec{}) events.push_back(7); vectorA={}; }
    void resetVectorB() { if(vectorB!=Vec{}) events.push_back(8); vectorB={}; }
    void snapEyeAngles(const Vec& value) { eyeAngles=value; events.push_back(9); }
};
void run() { Host host; require(sourceMapPlaceSpawnCandidate(host)==1,"owned selected entity"); }
#endif

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected installed server or -"); (void)argv;
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
        PrivateX86BoundaryObserverFixture observer{{0x10019790,0x100c3040,0x100c0670,0x100c31b0},boundary}; activeObserver=&observer;
#endif
        unsigned cases=0;
        for(int originCase=0;originCase<3;++originCase) for(int hitCase=0;hitCase<3;++hitCase)
        for(bool angles:{false,true}) for(unsigned flags=0;flags<4;++flags) {
            spawnOrigin={float(originCase)*60.f-25.f,32.f,float(originCase)*20.f+100.f};
            traceEnd={float(hitCase)*15.f+3.f,-7.f,float(hitCase)*10.f-60.f};
            spawnAngles=angles?Vec{10.f,-90.f,5.f}:Vec{};
            vectorA=flags&1?Vec{1,2,3}:Vec{}; vectorB=flags&2?Vec{4,5,6}:Vec{};
            events.clear();
#ifndef _WIN64
            observer.entries={0x10019790,0x100c3040,0x100c0670,0x100c31b0}; observer.configure(true);
#endif
            run();
#ifndef _WIN64
            observer.configure(false);
#endif
            auto expectedPosition=traceEnd; expectedPosition[2]+=1.f;
            std::vector<unsigned> sequence={1,2,2,3,4,5,6}; if(flags&1) sequence.push_back(7); if(flags&2) sequence.push_back(8); sequence.push_back(9);
            require(position==expectedPosition && localAngles==spawnAngles && eyeAngles==spawnAngles && vectorA==Vec{} && vectorB==Vec{} && events==sequence,"spawn placement policy mismatch"); ++cases;
        }
#ifndef _WIN64
        image.verify();
#endif
        std::cout<<"{\"cases\":"<<cases<<",\"passed\":true}\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
