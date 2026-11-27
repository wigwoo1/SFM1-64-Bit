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
#include "source_map_simulation_admission_candidate.h"

void require(bool v,const char* s) { if(!v) throw std::runtime_error(s); }
struct Query { uint32_t handle,flags; int base,contexts,tick,capacity; bool nullPointer; } query{};
struct Result {
    std::array<uint16_t,8192> positions{};
    std::vector<SourceMapSimulationAdmissionCandidate::Entry> entries;
    std::vector<uintptr_t> due;
} result;
const std::vector<int> contextTicks={9,2,-1};

#ifndef _WIN64
#include "native_private_world_image_fixture.h"
template<class T> T& at(uintptr_t p) { return *reinterpret_cast<T*>(p); }
uint32_t handle=0;
uint32_t* __fastcall getHandle(void*,void*) { return &handle; }
void run() {
    alignas(4) std::array<uint8_t,0x4020> manager{};
    alignas(4) std::array<uint8_t,0x140> actor{};
    std::array<uintptr_t,3> table{};
    std::array<uint32_t,16> nodes{};
    std::array<uint32_t,12> contexts{};
    std::array<uint8_t,0x1c> globals{};
    const auto m=reinterpret_cast<uintptr_t>(manager.data()), a=reinterpret_cast<uintptr_t>(actor.data());
    for(unsigned i=0;i<8192;++i) at<uint16_t>(m+4+2*i)=0xffff;
    // A spare-capacity list: append, update and fast removal run actual code.
    for(unsigned i=0;i<3;++i) { at<uint16_t>(m+4+2*i)=uint16_t(i); nodes[2*i]=i; nodes[2*i+1]=i==0?0:i==1?7:1; }
    at<uintptr_t>(m+0x4004)=reinterpret_cast<uintptr_t>(nodes.data());
    at<int>(m+0x4008)=8; at<int>(m+0x400c)=-1; at<int>(m+0x4010)=3;
    at<uintptr_t>(m+0x4014)=reinterpret_cast<uintptr_t>(nodes.data());
    handle=query.handle; table[2]=reinterpret_cast<uintptr_t>(getHandle);
    at<uintptr_t>(a)=reinterpret_cast<uintptr_t>(table.data()); at<uint32_t>(a+0x11c)=query.flags;
    at<int>(a+0xb4)=query.base; at<int>(a+0x94)=query.contexts;
    at<uintptr_t>(a+0x88)=reinterpret_cast<uintptr_t>(contexts.data());
    for(unsigned i=0;i<3;++i) contexts[4*i+2]=uint32_t(contextTicks[i]);
    const auto actorBefore=actor;
    reinterpret_cast<void (__thiscall*)(void*,void*)>(0x10110c70)(manager.data(),actor.data());
    require(actor==actorBefore,"admission mutated actor");
    for(unsigned i=0;i<8192;++i) result.positions[i]=at<uint16_t>(m+4+2*i);
    const auto count=at<int>(m+0x4010); require(count>=0 && count<=4,"bad native count");
    result.entries.clear();
    for(int i=0;i<count;++i) result.entries.push_back({uint16_t(nodes[2*i]),int(nodes[2*i+1])});
    at<int>(reinterpret_cast<uintptr_t>(globals.data())+0x18)=query.tick;
    at<uintptr_t>(0x10635ca0)=reinterpret_cast<uintptr_t>(globals.data());
    for(unsigned i=0;i<4;++i) at<uintptr_t>(0x1060eebc+16*i)=query.nullPointer && i==1?0:100+i;
    std::array<uintptr_t,8> output; output.fill(0xfedcba);
    const auto beforeManager=manager; const auto beforeNodes=nodes;
    const auto n=reinterpret_cast<int (__thiscall*)(void*,uintptr_t*,int)>(0x1010f430)(manager.data(),output.data(),query.capacity);
    require(n>=0 && n<=4,"bad admission output count");
    require(manager==beforeManager && nodes==beforeNodes,"due collector mutated list");
    for(unsigned i=unsigned(n);i<output.size();++i) require(output[i]==0xfedcba,"due output overrun");
    result.due.assign(output.begin(),output.begin()+n);
}
#else
void run() {
    SourceMapSimulationAdmissionCandidate list;
    list.entries={{0,0},{1,7},{2,1}}; for(uint16_t i=0;i<3;++i) list.positions[i]=i;
    list.refresh(query.handle,query.flags,[]{return sourceMapEarliestThinkTickCandidate(query.base,
        query.contexts?contextTicks:std::vector<int>{});});
    result.positions=list.positions; result.entries=list.entries;
    result.due=list.due(query.tick,query.capacity,[](uint16_t i){return uintptr_t(query.nullPointer && i==1?0:100+i);});
}
#endif

int main(int argc,char** argv) {
    try {
        require(argc==2,"expected server DLL or -"); (void)argv;
#ifndef _WIN64
        NativePrivateWorldImageFixture image(argv[1],0x6de6c4); image.seal();
#endif
        unsigned cases=0;
        for(uint32_t reference:{0u,1u,2u,3u,0x20001u,UINT32_MAX})
        for(uint32_t flags:{0u,1u,0x400000u,0x800000u,0xc00000u,0xc00001u})
        for(int base:{-1,0,1,5}) for(int contexts:{0,3})
        for(int tick:{-1,0,2,10}) for(int capacity:{-1,0,1,2,8}) for(bool nullPointer:{false,true}) {
            query={reference,flags,base,contexts,tick,capacity,nullPointer};
            std::vector<SourceMapSimulationAdmissionCandidate::Entry> expected={{0,0},{1,7},{2,1}};
            const bool admitted=!(flags&1) && reference!=UINT32_MAX;
            const auto index=uint16_t(reference);
            if(admitted) {
                if(flags==0xc00000u) { if(index<3) { expected[index]=expected.back(); expected.pop_back(); } }
                else {
                    int scheduled=0;
                    if(flags&0x800000u) scheduled=contexts?(base==1?1:2):(base>0?base:-1);
                    if(index<3) expected[index].tick=scheduled; else expected.push_back({index,scheduled});
                }
            }
            run();
            require(result.entries.size()==expected.size(),"list count mismatch");
            std::array<uint16_t,8192> map; map.fill(0xffff);
            std::vector<uintptr_t> due;
            for(unsigned i=0;i<expected.size();++i) {
                require(result.entries[i].index==expected[i].index && result.entries[i].tick==expected[i].tick,"list record mismatch");
                map[expected[i].index]=uint16_t(i);
                if(int(i)<capacity && expected[i].tick<=tick) due.push_back(nullPointer && expected[i].index==1?0:100+expected[i].index);
            }
            require(result.positions==map && result.due==due,"index map or due order mismatch"); ++cases;
        }
#ifndef _WIN64
        image.verify();
#endif
        std::cout<<"{\"cases\":"<<cases<<",\"passed\":true}\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
