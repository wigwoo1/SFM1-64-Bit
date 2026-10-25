#include "phong_vertex_variant_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <iterator>
#ifdef SFM64_NATIVE_PHONG_DEFAULTS
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static int32_t levels[2]{},fog=0;static uint32_t rawMorph=0,hwCalls=0,morphCalls=0,fogCalls=0;
static int32_t __fastcall hardware(void*,void*){if(hwCalls>=2)throw std::runtime_error("hardware query extent");return levels[hwCalls++];}
static uint32_t __fastcall morph(void*,void*){++morphCalls;return rawMorph;}
static int32_t __fastcall fogQuery(void*,void*){++fogCalls;return fog;}
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("fresh defaults output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("defaults fixture IO");
#ifdef SFM64_NATIVE_PHONG_DEFAULTS
    static_assert(sizeof(void*)==4,"private x86");NativePrivateWorldImageFixture image(argv[1],0xf0000);
    std::array<uintptr_t,18>hwTable{};hwTable[17]=reinterpret_cast<uintptr_t>(&hardware);const auto tablePointer=reinterpret_cast<uintptr_t>(hwTable.data());
    std::array<uintptr_t,1>hwObject{tablePointer};const auto objectPointer=reinterpret_cast<uintptr_t>(hwObject.data());std::memcpy(image.base+0xdb9d8,&objectPointer,4);image.seal();
    std::array<uintptr_t,40>apiTable{};apiTable[2]=reinterpret_cast<uintptr_t>(&fogQuery);apiTable[39]=reinterpret_cast<uintptr_t>(&morph);
    std::array<uintptr_t,1>apiObject{reinterpret_cast<uintptr_t>(apiTable.data())};
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    std::array<uint32_t,6>q{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),24)){
        if(q[0]>2)throw std::runtime_error("defaults kind");int32_t first=0,second=0,fogValue=0;std::memcpy(&first,&q[1],4);std::memcpy(&second,&q[2],4);std::memcpy(&fogValue,&q[5],4);
        std::array<uint32_t,8>result{};result.fill(0xa5a5a5a5);
#ifdef SFM64_NATIVE_PHONG_DEFAULTS
        levels[0]=first;levels[1]=second;fog=fogValue;rawMorph=q[4];hwCalls=morphCalls=fogCalls=0;
        std::array<uint32_t,7>guard{};guard.fill(0xa5a5a5a5);std::array<uint8_t,32>flags{};std::memcpy(flags.data()+8,&q[3],4);std::array<void*,1>params{flags.data()};const auto beforeFlags=flags;
        if(q[0]<2){using Fn=void*(__thiscall*)(void*,void*,void*);const auto returned=reinterpret_cast<Fn>(image.base+(q[0]?0x37750:0x37690))(guard.data()+1,nullptr,params.data());if(returned!=guard.data()+1)throw std::runtime_error("static constructor return");}
        else{using Fn=void*(__thiscall*)(void*,void*);const auto returned=reinterpret_cast<Fn>(image.base+0x54370)(guard.data()+1,apiObject.data());if(returned!=guard.data()+1)throw std::runtime_error("dynamic constructor return");}
        if(guard.front()!=0xa5a5a5a5||guard.back()!=0xa5a5a5a5||flags!=beforeFlags)throw std::runtime_error("defaults mutation/canary");std::memcpy(result.data(),guard.data()+1,20);result[5]=hwCalls;result[6]=morphCalls;result[7]=fogCalls;
#else
        if(q[0]<2){const auto fields=phongVertexStaticDefaultsCandidate(q[0]?30:20,first,second,q[3]);std::memcpy(result.data(),fields.data(),16);result[5]=q[0]?1:2;result[6]=result[7]=0;}
        else{const auto fields=phongVs30DynamicDefaultsCandidate(q[4],fogValue);std::memcpy(result.data(),fields.data(),20);result[5]=0;result[6]=result[7]=1;}
#endif
        out.write(reinterpret_cast<const char*>(result.data()),32);++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("defaults framing");out.flush();if(!out)throw std::runtime_error("defaults output failure");
#ifdef SFM64_NATIVE_PHONG_DEFAULTS
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS phong_defaults_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
