#include "material_numeric_value_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#ifdef SFM64_NATIVE_MATERIAL_NUMERIC
#define NOMINMAX
#include <windows.h>
#include <xmmintrin.h>
#include "native_private_world_image_fixture.h"
static unsigned queueCalls=0;
static void __fastcall unexpectedNumericService(void*,void*){ExitProcess(92);}
static void* __fastcall noQueue(void*,void*){++queueCalls;return nullptr;}
#endif
static_assert(sizeof(MaterialNumericValueCandidate)==24,"numeric wire value");
static void analytical(){
    using Op=MaterialNumericOpCandidate;unsigned checks=0,refusals=0;
    auto need=[&](bool b){if(!b)throw std::runtime_error("numeric analytical failure");++checks;};
    need(materialNumericTruncateCandidate(0x7fc12345)==0x80000000u);
    need(materialNumericTruncateCandidate(0xcf000000)==0x80000000u);
    need(materialNumericTruncateCandidate(0x4effffff)==0x7fffff80u);
    need(materialNumericTruncateCandidate(0xbfcccccd)==0xffffffffu);
    need(materialNumericIntFloatCandidate(0x7fffffff)==0x4f000000u);
    need(materialNumericIntFloatCandidate(0x80000000)==0xcf000000u);
    MaterialNumericValueCandidate v{0x12345606,0,{1,2,3,4}};
    need(materialNumericSetCandidate(v,Op::SetInt,{3,0,0,0})&&v.vector==std::array<uint32_t,4>{0x40400000,0x40400000,0x40400000,0x40400000});
    v={0x87654300,77,{0x80000000,11,12,13}};const auto zero=v;
    need(materialNumericSetCandidate(v,Op::SetFloat,{0,0,0,0})&&!std::memcmp(&v,&zero,24));
    v={0x12345642,99,{0x3f800000,0x40000000,0,0}};const auto vector=v;
    need(materialNumericSetCandidate(v,Op::SetVector2,{0x3f800000,0x40000000,8,9})&&!std::memcmp(&v,&vector,24));
    need(materialNumericSetCandidate(v,Op::SetUndefined,{})&&v.integer==99&&v.vector==vector.vector);
    for(uint32_t type:{1u,3u,5u,7u,8u,9u,10u,11u,12u,13u,14u,15u}){
        v.typeAndMetadata=0x12345600|type;const auto before=v;
        need(!materialNumericSetCandidate(v,Op::SetInt,{})&&!std::memcmp(&v,&before,24));++refusals;
    }
    for(auto bad:std::array<std::array<uint32_t,2>,4>{{{7,0},{5,0},{5,5},{0,1}}}){
        v.typeAndMetadata=6;const auto before=v;
        need(!materialNumericSetCandidate(v,Op(bad[0]),{},bad[1])&&!std::memcmp(&v,&before,24));++refusals;
    }
    std::cout<<"PASS numeric_analytical_checks="<<checks<<" refusal_cases="<<refusals<<'\n';
}
int main(int argc,char**argv){try{
    if(argc==2&&std::string(argv[1])=="--test"){analytical();return 0;}
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("library queries fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);
    if(!in||!out)throw std::runtime_error("numeric fixture IO");
#ifdef SFM64_NATIVE_MATERIAL_NUMERIC
    static_assert(sizeof(void*)==4,"private reference only");NativePrivateWorldImageFixture image(argv[1],0x100000);
    std::array<uintptr_t,145>serviceTable{};serviceTable.fill(reinterpret_cast<uintptr_t>(&unexpectedNumericService));
    serviceTable[144]=reinterpret_cast<uintptr_t>(&noQueue);uintptr_t service=reinterpret_cast<uintptr_t>(serviceTable.data());
    const auto servicePointer=reinterpret_cast<uintptr_t>(&service);std::memcpy(image.base+0xfc7b0,&servicePointer,4);image.seal();
    const unsigned oldMxcsr=_mm_getcsr();
#else
    static_assert(sizeof(void*)==8,"owned native x64");
#endif
    std::array<uint32_t,12>q{};size_t cases=0,changedCases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        MaterialNumericValueCandidate value;std::memcpy(&value,q.data()+2,24);const auto before=value;
        const std::array<uint32_t,4>a{q[8],q[9],q[10],q[11]};auto admitted=value;
        if(!materialNumericSetCandidate(admitted,MaterialNumericOpCandidate(q[0]),a,q[1]))throw std::runtime_error("numeric unsupported input");
#ifdef SFM64_NATIVE_MATERIAL_NUMERIC
        _mm_setcsr(0x1f80);queueCalls=0;
        std::array<uint8_t,40>object{};const uintptr_t table=reinterpret_cast<uintptr_t>(image.base+0x98f34);
        std::memcpy(object.data(),&table,4);std::memcpy(object.data()+8,&value.integer,4);
        std::memcpy(object.data()+12,value.vector.data(),16);std::memcpy(object.data()+28,&value.typeAndMetadata,4);
        const auto rawBefore=object;constexpr unsigned slots[]{4,3,11,10,9,12,18};uintptr_t method=0;
        std::memcpy(&method,image.base+0x98f34+4*slots[q[0]],4);
        // x86 thiscall float arguments are forwarded as exact stack words,
        // avoiding host-side NaN conversions before the original method reads them.
        if(q[0]<2){using Fn=void(__thiscall*)(void*,uint32_t);reinterpret_cast<Fn>(method)(object.data(),a[0]);}
        else if(q[0]==2){using Fn=void(__thiscall*)(void*,uint32_t,uint32_t);reinterpret_cast<Fn>(method)(object.data(),a[0],a[1]);}
        else if(q[0]==3){using Fn=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t);reinterpret_cast<Fn>(method)(object.data(),a[0],a[1],a[2]);}
        else if(q[0]==4){using Fn=void(__thiscall*)(void*,uint32_t,uint32_t,uint32_t,uint32_t);reinterpret_cast<Fn>(method)(object.data(),a[0],a[1],a[2],a[3]);}
        else if(q[0]==5){using Fn=void(__thiscall*)(void*,const void*,int);reinterpret_cast<Fn>(method)(object.data(),a.data(),int(q[1]));}
        else{using Fn=void(__thiscall*)(void*);reinterpret_cast<Fn>(method)(object.data());}
        if(queueCalls!=1)throw std::runtime_error("immediate queue-query contract");
        for(size_t i=0;i<object.size();++i)if((i<8||i>=32)&&object[i]!=rawBefore[i])throw std::runtime_error("native pointer/owner mutation");
        std::memcpy(&value.integer,object.data()+8,4);std::memcpy(value.vector.data(),object.data()+12,16);
        std::memcpy(&value.typeAndMetadata,object.data()+28,4);
#else
        value=admitted;
#endif
        const uint32_t changed=std::memcmp(&value,&before,24)!=0;
        out.write(reinterpret_cast<const char*>(&value),24);out.write(reinterpret_cast<const char*>(&changed),4);++cases;changedCases+=changed;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("numeric query framing");out.flush();if(!out)throw std::runtime_error("numeric output");
#ifdef SFM64_NATIVE_MATERIAL_NUMERIC
    _mm_setcsr(oldMxcsr);image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS material_numeric_cases="<<cases<<" changed_value_records="<<changedCases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
