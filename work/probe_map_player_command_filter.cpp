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
#include "source_map_player_command_filter_candidate.h"

void require(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
using Command=SourceMapPlayerCommandFilterInputCandidate;
struct Query { float time; unsigned bits; } query{};
Command command{};
std::array<float,3> cached{};
bool optionFlag=false;
uint8_t commandFlag=0;
std::vector<unsigned> events;
uintptr_t actor=0;
void movement(Command& value) {
    events.push_back(2);
    if(query.bits&64) { value.buttons^=0x400; value.movement[0]+=4.f; }
}

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
#include "private_x86_boundary_observer_fixture.h"
template<class T> T& at(uintptr_t p) { return *reinterpret_cast<T*>(p); }
Command readCommand(uintptr_t p) {
    Command value;
    for(unsigned i=0;i<3;++i) { value.view[i]=at<float>(p+0xc+4*i); value.movement[i]=at<float>(p+0x18+4*i); }
    value.buttons=at<uint32_t>(p+0x24); value.impulse=at<uint8_t>(p+0x28); return value;
}
void writeCommand(uintptr_t p,const Command& value) {
    for(unsigned i=0;i<3;++i) { at<float>(p+0xc+4*i)=value.view[i]; at<float>(p+0x18+4*i)=value.movement[i]; }
    at<uint32_t>(p+0x24)=value.buttons; at<uint8_t>(p+0x28)=value.impulse;
}
void __fastcall runMovement(void* self,void*,void* player,void* cmd,void* helper) {
    require(reinterpret_cast<uintptr_t>(self)==0x106d8898 && reinterpret_cast<uintptr_t>(player)==actor &&
        reinterpret_cast<uintptr_t>(helper)==0x12340000,"movement forwarding arguments");
    command=readCommand(reinterpret_cast<uintptr_t>(cmd)); movement(command); writeCommand(reinterpret_cast<uintptr_t>(cmd),command);
}
void boundary(unsigned slot,CONTEXT& c) {
    require(slot==0 && c.Ecx==0x1063e2c0 && at<int>(c.Esp+4)==0,"secondary option setter request");
    require(at<uint8_t>(actor+0xf4c)==0,"player option cleared after request");
    events.push_back(1); PrivateX86BoundaryObserverFixture::returnCdecl(c); c.Esp+=4;
}
void run() {
    alignas(4) std::array<uint8_t,0x1000> bytes{};
    std::array<uint8_t,0x58> cmd; cmd.fill(0xa5);
    std::array<uint8_t,0x4c> globals{};
    std::array<uint8_t,0x34> primary{},secondary{};
    std::array<uintptr_t,2> table{};
    actor=reinterpret_cast<uintptr_t>(bytes.data()); const auto p=reinterpret_cast<uintptr_t>(cmd.data());
    writeCommand(p,command);
    at<int>(actor+0xbb8)=query.bits&1?1:0; at<uint32_t>(actor+0x120)=query.bits&2?0x20:0;
    at<uint8_t>(actor+0xf4c)=optionFlag; at<uint8_t>(actor+0xd1c)=commandFlag;
    for(unsigned i=0;i<3;++i) at<float>(actor+0xba8+4*i)=cached[i];
    const auto g=reinterpret_cast<uintptr_t>(globals.data());
    at<float>(g+0xc)=query.time; at<int>(g+0x48)=query.bits&8?1:0;
    at<int>(reinterpret_cast<uintptr_t>(primary.data())+0x30)=query.bits&4?1:0;
    at<int>(reinterpret_cast<uintptr_t>(secondary.data())+0x30)=query.bits&32?1:0;
    at<uintptr_t>(0x10635ca0)=g; at<uintptr_t>(0x1066f914)=reinterpret_cast<uintptr_t>(primary.data());
    at<uintptr_t>(0x1063e2c4)=reinterpret_cast<uintptr_t>(secondary.data());
    table[1]=reinterpret_cast<uintptr_t>(runMovement); at<uintptr_t>(0x106d8898)=reinterpret_cast<uintptr_t>(table.data());
    const auto before=bytes; const auto beforeCommand=cmd;
    const auto entry=at<uintptr_t>(0x1050eaac+0x700);
    reinterpret_cast<void (__thiscall*)(void*,void*,void*)>(entry)(bytes.data(),cmd.data(),reinterpret_cast<void*>(0x12340000));
    command=readCommand(p); optionFlag=at<uint8_t>(actor+0xf4c)!=0; commandFlag=at<uint8_t>(actor+0xd1c);
    for(unsigned i=0;i<3;++i) cached[i]=at<float>(actor+0xba8+4*i);
    auto expected=before; const auto e=reinterpret_cast<uintptr_t>(expected.data());
    at<uint8_t>(e+0xd1c)=commandFlag; at<uint8_t>(e+0xf4c)=optionFlag;
    for(unsigned i=0;i<3;++i) at<float>(e+0xba8+4*i)=cached[i];
    require(bytes==expected,"unrelated player storage changed");
    auto expectedCommand=beforeCommand; writeCommand(reinterpret_cast<uintptr_t>(expectedCommand.data()),command);
    require(cmd==expectedCommand,"unrelated command storage changed");
}
#else
struct Host {
    void resetCommandFlag() { commandFlag=0; }
    bool inputAccumulatorIsZero() { return !(query.bits&1); }
    void cacheView(const std::array<float,3>& v) { cached=v; }
    bool suppressedByPlayerFlag() { return query.bits&2; }
    bool optionEnabled() { return query.bits&4; }
    bool globalMode() { return query.bits&8; }
    float serverTime() { return query.time; }
    std::array<float,3> cachedView() { return cached; }
    bool playerOptionFlag() { return optionFlag; }
    bool secondaryOptionEnabled() { return query.bits&32; }
    void clearPlayerOptionFlag() { optionFlag=false; }
    void requestSecondaryOptionZero() { events.push_back(1); }
    void runMovement(Command& value) { movement(value); }
};
void run() { Host host; sourceMapFilterPlayerCommandCandidate(command,host); }
#endif

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected installed server or -"); (void)argv;
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
        PrivateX86BoundaryObserverFixture observer{{0x10386700,0x10002800,0x10002600,0x10002400},boundary}; observer.configure(true);
#endif
        unsigned cases=0;
        for(float time:{0.f,2.999f,3.f,4.f}) for(unsigned bits=0;bits<128;++bits) {
            query={time,bits}; command={{17.f,-90.f,5.f},{1.f,2.f,3.f},0x80,7}; cached={9.f,8.f,7.f};
            optionFlag=bool(bits&16); commandFlag=0xa5;
            Command expected=command; auto expectedCached=(bits&1)?cached:command.view;
            bool expectedFlag=optionFlag; std::vector<unsigned> expectedEvents;
            const bool suppress=(bits&2) || (!(bits&4) && !(bits&8) && time<3.f);
            if(suppress) { expected.movement={}; expected.view=expectedCached; expected.buttons=0; expected.impulse=0; }
            else if(bits&16) { if(bits&32) { expectedFlag=false; expectedEvents.push_back(1); } else expected.buttons|=4; }
            expectedEvents.push_back(2);
            if(bits&64) { expected.buttons^=0x400; expected.movement[0]+=4.f; }
            events.clear(); run();
            require(command.view==expected.view && command.movement==expected.movement && command.buttons==expected.buttons &&
                command.impulse==expected.impulse && cached==expectedCached && optionFlag==expectedFlag && commandFlag==0 &&
                events==expectedEvents,"player command filter mismatch"); ++cases;
        }
#ifndef _WIN64
        observer.configure(false); image.verify();
#endif
        std::cout<<"{\"cases\":"<<cases<<",\"passed\":true}\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
