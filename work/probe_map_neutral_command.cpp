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
#include "source_map_neutral_command_candidate.h"

void require(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
using Command=std::array<uint8_t,0x58>;
struct Clock { float current,frame; } clockState{};
struct Query { float time,interval; int previous; bool mutate; int angles; } query{};
std::vector<unsigned> events;
Command stored{},processed{};
std::array<float,3> angles{};
int tickCounter=0;
int accumulator=88;
uintptr_t player=0,processor=0;
template<class T> T& at(uintptr_t p) { return *reinterpret_cast<T*>(p); }
int roundedTick() { const float q=query.time/query.interval; const float rounded=q+0.5f; return int(rounded); }
bool field(unsigned i) {
    return (i>=4 && i<0x29) || (i>=0x2c && i<0x3d) || (i>=0x40 && i<0x58);
}
void commandWork(Command& command) {
    for(unsigned i=4;i<command.size();++i) {
        const bool angle=i>=0xc && i<0x18;
        if(!angle && field(i)) require(command[i]==0,"non-neutral command field");
    }
    require(std::memcmp(command.data()+0xc,angles.data(),12)==0,"command view angles");
    events.push_back(4);
    if(query.mutate) {
        int number=77,buttons=0x1234; std::memcpy(command.data()+4,&number,4); std::memcpy(command.data()+0x24,&buttons,4);
        clockState={9.f,0.25f};
    }
    processed=command;
}

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
uintptr_t globals=0;
const float* __fastcall viewAngles(void* self,void*) {
    require(reinterpret_cast<uintptr_t>(self)==player && at<int>(player+0xbb8)==0,"view getter state");
    events.push_back(1); return angles.data();
}
void __fastcall changedTick(void* self,void*,void* address) {
    require(reinterpret_cast<uintptr_t>(self)==player && reinterpret_cast<uintptr_t>(address)==player+0xf6c,"tick change field");
    require(at<int>(player+0xf6c)==query.previous,"tick notice after store"); events.push_back(2);
}
void __fastcall hostPlayer(void* self,void*,void* owner) {
    require(reinterpret_cast<uintptr_t>(self)==processor,"command processor owner");
    require(owner==nullptr || reinterpret_cast<uintptr_t>(owner)==player,"command host argument");
    if(!owner) require(at<float>(globals+0xc)==query.time && at<float>(globals+0x10)==0.125f,"host cleared before clock restore");
    events.push_back(owner?3:6);
}
void __fastcall runCommand(void* self,void*,uint8_t* command,void* move) {
    require(reinterpret_cast<uintptr_t>(self)==player && reinterpret_cast<uintptr_t>(move)==processor,"command execution owners");
    require(at<uint32_t>(reinterpret_cast<uintptr_t>(command))==0x104953fc,"native command vtable");
    Command copy; std::memcpy(copy.data(),command,copy.size()); commandWork(copy); std::memcpy(command,copy.data(),copy.size());
    at<float>(globals+0xc)=clockState.current; at<float>(globals+0x10)=clockState.frame;
}
void run() {
    alignas(4) std::array<uint8_t,0x1000> bytes{};
    std::array<uintptr_t,0x784/4> table{};
    std::array<uintptr_t,2> processorTable{};
    uintptr_t processorInstance=reinterpret_cast<uintptr_t>(processorTable.data());
    std::array<uint8_t,0x20> globalBytes{};
    player=reinterpret_cast<uintptr_t>(bytes.data()); processor=reinterpret_cast<uintptr_t>(&processorInstance);
    globals=reinterpret_cast<uintptr_t>(globalBytes.data());
    table[0x224/4]=reinterpret_cast<uintptr_t>(viewAngles); table[0x700/4]=reinterpret_cast<uintptr_t>(runCommand);
    table[0x780/4]=reinterpret_cast<uintptr_t>(changedTick); processorTable[1]=reinterpret_cast<uintptr_t>(hostPlayer);
    at<uintptr_t>(player)=reinterpret_cast<uintptr_t>(table.data()); at<int>(player+0xbb8)=88;
    at<int>(player+0xf6c)=query.previous; std::memset(bytes.data()+0xeb0,0xa5,0x58);
    at<float>(globals+0xc)=query.time; at<float>(globals+0x10)=0.125f; at<float>(globals+0x1c)=query.interval;
    at<uintptr_t>(0x10635ca0)=globals; at<uintptr_t>(0x10639f40)=processor;
    const auto before=bytes; const auto beforeGlobals=globalBytes;
    reinterpret_cast<void (__thiscall*)(void*)>(0x101c5650)(bytes.data());
    tickCounter=at<int>(player+0xf6c); std::memcpy(stored.data(),bytes.data()+0xeb0,stored.size());
    accumulator=at<int>(player+0xbb8);
    clockState={at<float>(globals+0xc),at<float>(globals+0x10)};
    auto expected=before; const auto p=reinterpret_cast<uintptr_t>(expected.data());
    at<int>(p+0xbb8)=0; at<int>(p+0xf6c)=tickCounter;
    for(unsigned i=0;i<stored.size();++i) if(field(i)) expected[0xeb0+i]=processed[i];
    require(bytes==expected,"neutral command changed unrelated player bytes or copied padding/vtable");
    require(globalBytes==beforeGlobals,"server globals not restored");
}
#else
struct Host {
    void resetInputAccumulator() { accumulator=0; }
    Clock clock() { return clockState; }
    const std::array<float,3>& viewAngles() { events.push_back(1); return angles; }
    Command neutralCommand(const std::array<float,3>& value) { Command command{}; std::memcpy(command.data()+0xc,value.data(),12); return command; }
    int roundedServerTick() { return roundedTick(); }
    int tickCounter() { return ::tickCounter; }
    void setTickCounter(int v) { events.push_back(2); ::tickCounter=v; }
    void setCommandHost(bool v) {
        if(!v) require(clockState.current==query.time && clockState.frame==0.125f,"owned clock restore ordering");
        events.push_back(v?3:6);
    }
    void runCommand(Command& command) { commandWork(command); }
    void storeLastCommand(const Command& command) { for(unsigned i=0;i<stored.size();++i) if(field(i)) stored[i]=command[i]; }
    void restoreClock(Clock saved) { clockState=saved; }
};
void run() { accumulator=88; tickCounter=query.previous; stored.fill(0xa5); Host host; sourceMapRunNeutralCommandCandidate(host); }
#endif

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected server DLL or -"); (void)argv;
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
#endif
        unsigned cases=0;
        for(float time:{0.f,1.25f,-0.03125f}) for(float interval:{0.03125f,0.0625f})
        for(int previous:{-1,0,40}) for(bool mutate:{false,true}) for(int angleCase:{0,1}) {
            query={time,interval,previous,mutate,angleCase}; angles=angleCase?std::array<float,3>{17.f,-90.f,5.f}:std::array<float,3>{};
            clockState={time,0.125f}; events.clear(); run();
            std::vector<unsigned> expected={1}; if(previous!=roundedTick()) expected.push_back(2);
            expected.insert(expected.end(),{3,4,6});
            require(events==expected && accumulator==0 && tickCounter==roundedTick() && clockState.current==time && clockState.frame==0.125f,"neutral command flow mismatch");
            for(unsigned i=0;i<stored.size();++i) require(stored[i]==(field(i)?processed[i]:0xa5),"stored command fields mismatch");
            ++cases;
        }
#ifndef _WIN64
        image.verify();
#endif
        std::cout<<"{\"cases\":"<<cases<<",\"passed\":true}\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
