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
#include "source_map_observer_mode_policy_candidate.h"

void require(bool value,const char* message) { if (!value) throw std::runtime_error(message); }
struct Query { int requested,team,previous; bool dead,forced,restricted,valid,target,networked; };
struct State { int mode,last; float time; uint8_t dirty; };
Query query{}; State state{};
std::vector<unsigned> events;
uintptr_t actor=0,target=0;
void fov() { events.push_back(1); }
uintptr_t observerTarget() { events.push_back(2); return target; }
bool accepts(uintptr_t value) { require(value==target,"eligibility target"); events.push_back(3); return query.valid; }
void apply(uintptr_t value) { require(value==target,"applied target"); events.push_back(4); }
void zero() { events.push_back(5); }
void movement(int value) { require(value==0 || value==10,"movement request"); events.push_back(60+value); }
void check() { events.push_back(7); }

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T> T& at(uintptr_t address) { return *reinterpret_cast<T*>(address); }
void owner(void* self) { require(reinterpret_cast<uintptr_t>(self)==actor,"observer mode owner"); }
void* __fastcall targetCall(void* self,void*) { owner(self); return reinterpret_cast<void*>(observerTarget()); }
bool __fastcall acceptsCall(void* self,void*,void* value) { owner(self); return accepts(reinterpret_cast<uintptr_t>(value)); }
bool __fastcall applyCall(void* self,void*,void* value) { owner(self); apply(reinterpret_cast<uintptr_t>(value)); return false; }
void __fastcall zeroCall(void* self,void*,const float* value) {
    owner(self); require(value[0]==0 && value[1]==0 && value[2]==0,"zero view offset arguments"); zero();
}
void __fastcall checkCall(void* self,void*) { owner(self); check(); }
void boundary(unsigned slot,CONTEXT& c) {
    using Observer=PrivateX86BoundaryObserverFixture;
    require(c.Ecx==actor,"mode boundary owner");
    if (slot==0) {
        require(at<uint32_t>(c.Esp+4)==actor && at<uint32_t>(c.Esp+8)==0 &&
                at<uint32_t>(c.Esp+12)==0 && at<uint32_t>(c.Esp+16)==0,"FOV request arguments");
        fov(); Observer::returnCdecl(c); c.Esp+=16;
    } else if (slot==1) {
        require(at<uint32_t>(c.Esp+8)==0,"movement secondary argument");
        movement(at<int32_t>(c.Esp+4)); Observer::returnCdecl(c); c.Esp+=8;
    } else throw std::runtime_error("unexpected mode boundary");
}
bool run() {
    alignas(4) std::array<uint8_t,0x1900> bytes{};
    std::array<uintptr_t,0x664/4> table{};
    std::array<uint8_t,0x34> cameraVariable{},restrictedVariable{};
    std::array<uint8_t,0x30> entities{};
    std::array<uint8_t,0x10> globals{};
    int targetObject=0;
    actor=reinterpret_cast<uintptr_t>(bytes.data());
    target=query.target?reinterpret_cast<uintptr_t>(&targetObject):0;
    table[0x64c/4]=reinterpret_cast<uintptr_t>(targetCall);
    table[0x65c/4]=reinterpret_cast<uintptr_t>(acceptsCall);
    table[0x644/4]=reinterpret_cast<uintptr_t>(applyCall);
    table[0x240/4]=reinterpret_cast<uintptr_t>(zeroCall);
    table[0x660/4]=reinterpret_cast<uintptr_t>(checkCall);
    at<uintptr_t>(actor)=reinterpret_cast<uintptr_t>(table.data());
    at<int>(actor+0x1ec)=query.team; at<uint8_t>(actor+0xd8)=query.dead?2:0;
    at<int>(actor+0xc88)=query.previous; at<int>(actor+0xc9c)=99;
    at<float>(actor+0x1864)=-2.f;
    at<uint8_t>(actor+0x54)=query.networked; at<uint8_t>(actor+0x58)=0xa0;
    at<uint32_t>(actor+0xca0)=query.target?0x00020001:UINT32_MAX;
    at<uintptr_t>(reinterpret_cast<uintptr_t>(entities.data())+20)=target;
    at<uint32_t>(reinterpret_cast<uintptr_t>(entities.data())+24)=2;
    at<uintptr_t>(0x105a01e0)=reinterpret_cast<uintptr_t>(entities.data());
    at<int>(reinterpret_cast<uintptr_t>(cameraVariable.data())+0x30)=query.forced;
    at<int>(reinterpret_cast<uintptr_t>(restrictedVariable.data())+0x30)=query.restricted;
    at<uintptr_t>(0x1063803c)=reinterpret_cast<uintptr_t>(cameraVariable.data());
    at<uintptr_t>(0x1066620c)=reinterpret_cast<uintptr_t>(restrictedVariable.data());
    at<float>(reinterpret_cast<uintptr_t>(globals.data())+0xc)=1.25f;
    at<uintptr_t>(0x10635ca0)=reinterpret_cast<uintptr_t>(globals.data());
    const auto before=bytes;
    const auto entry=at<uintptr_t>(0x1050eaac+0x63c);
    const auto result=reinterpret_cast<uint32_t (__thiscall*)(void*,int)>(entry)(bytes.data(),query.requested);
    state={at<int>(actor+0xc88),at<int>(actor+0xc9c),at<float>(actor+0x1864),at<uint8_t>(actor+0x58)};
    auto expected=before; const auto address=reinterpret_cast<uintptr_t>(expected.data());
    at<int>(address+0xc88)=state.mode; at<int>(address+0xc9c)=state.last;
    at<float>(address+0x1864)=state.time; at<uint8_t>(address+0x58)=state.dirty;
    require(bytes==expected,"mode setter changed unrelated actor bytes");
    return static_cast<uint8_t>(result)!=0;
}
#else
struct Host {
    int team() { return query.team; }
    bool lifeStateIsTwo() { return query.dead; }
    bool forceCamera() { return query.forced; }
    int mode() { return state.mode; }
    void setLastMode(int value) { state.last=value; }
    void setMode(int value) { if (query.networked) state.dirty|=1; state.mode=value; }
    void markModeTime() { state.time=1.25f; }
    bool restrictedView() { return query.restricted; }
    uintptr_t observerTarget() { return ::observerTarget(); }
    bool accepts(uintptr_t value) { return ::accepts(value); }
    void resetFov() { fov(); }
    void zeroViewOffset() { zero(); }
    uintptr_t targetReference() { return target; }
    void applyTarget(uintptr_t value) { apply(value); }
    void move(int value) { movement(value); }
    void checkSettings() { check(); }
};
bool run() {
    state={query.previous,99,-2.f,0xa0}; target=query.target?1:0;
    Host host; return sourceMapSetObserverMode(query.requested,host);
}
#endif

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected installed server or -"); (void)argv;
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
        PrivateX86BoundaryObserverFixture observer{{0x100ddbe0,0x100c2710,0x10002800,0x10002600},boundary};
        observer.configure(true);
#endif
        unsigned cases=0;
        for (int requested : {-1,0,1,2,3,4,5,6,7}) for (int team : {1,2})
        for (int previous : {0,1,3,6}) for (unsigned flags=0;flags<64;++flags) {
            query={requested,team,previous,bool(flags&1),bool(flags&2),bool(flags&4),bool(flags&8),bool(flags&16),bool(flags&32)};
            State expected={previous,99,-2.f,0xa0}; std::vector<unsigned> sequence;
            const bool valid=requested>=0 && requested<=6;
            if (valid) {
                int chosen=requested;
                if (team==2 && chosen>3) {
                    if (query.dead && query.forced) chosen=5;
                    else if (chosen==6) chosen=4;
                }
                if (previous>1) expected.last=previous;
                const bool firstChange=chosen!=previous;
                if (query.restricted && chosen!=1 && chosen!=2 && team==2) {
                    sequence={2,3}; const int replacement=query.valid?4:1;
                    if (query.networked && chosen!=replacement) expected.dirty|=1;
                    chosen=replacement;
                }
                if (query.networked && firstChange) expected.dirty|=1;
                expected.mode=chosen; expected.time=1.25f;
                if (chosen==0 || chosen==1 || chosen==3) sequence.insert(sequence.end(),{1,5,60});
                else if (chosen==2 || chosen==6) sequence.insert(sequence.end(),{1,4,5,70});
                else sequence.insert(sequence.end(),{4,70});
                sequence.push_back(7);
            }
            events.clear(); const bool result=run();
            require(result==valid && state.mode==expected.mode && state.last==expected.last && state.time==expected.time &&
                    state.dirty==expected.dirty && events==sequence,"observer mode policy mismatch");
            ++cases;
        }
#ifndef _WIN64
        observer.configure(false); image.verify();
#endif
        std::cout << "{\"cases\":" << cases << ",\"passed\":true}\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
