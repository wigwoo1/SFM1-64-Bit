#include "material_vector_component_candidate.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <stdexcept>
#ifdef SFM64_NATIVE_PROXY_COMPONENT
#define NOMINMAX
#include <windows.h>
#include <vector>
#include <stdexcept>
#include <xmmintrin.h>
#include "native_private_world_image_fixture.h"
static unsigned queueCalls=0;
static void* __fastcall componentNoQueue(void*,void*){++queueCalls;return nullptr;}
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3]).good())throw std::runtime_error("library-or-absent query fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("Component IO");
#ifdef SFM64_NATIVE_PROXY_COMPONENT
    NativePrivateWorldImageFixture image(argv[1],0x100000);std::array<uintptr_t,145>methods{};methods[144]=reinterpret_cast<uintptr_t>(&componentNoQueue);uintptr_t object=reinterpret_cast<uintptr_t>(methods.data()),address=reinterpret_cast<uintptr_t>(&object);std::memcpy(image.base+0xfc7b0,&address,4);image.seal();const auto mx=_mm_getcsr();_mm_setcsr(0x1f80);
#else
    static_assert(sizeof(void*)==8,"Independent component implementation x64");
#endif
    std::array<uint32_t,8>q{};unsigned cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        MaterialNumericValueCandidate value;std::memcpy(&value,q.data(),24);const auto before=value;auto admitted=value;if(!materialVectorComponentCandidate(admitted,q[6],q[7]))throw std::runtime_error("Outside checked vector-component domain");
#ifdef SFM64_NATIVE_PROXY_COMPONENT
        std::array<uint8_t,40>raw{};const uintptr_t table=0x10098f34;std::memcpy(raw.data(),&table,4);std::memcpy(raw.data()+8,&value.integer,4);std::memcpy(raw.data()+12,value.vector.data(),16);std::memcpy(raw.data()+28,&value.typeAndMetadata,4);queueCalls=0;
        using Call=void(__thiscall*)(void*,uint32_t,uint32_t);reinterpret_cast<Call>(0x10019d90)(raw.data(),q[7],q[6]);if(queueCalls!=1)throw std::runtime_error("Real vector-component service call");std::memcpy(&value.integer,raw.data()+8,4);std::memcpy(value.vector.data(),raw.data()+12,16);std::memcpy(&value.typeAndMetadata,raw.data()+28,4);image.verify();
#else
        value=admitted;
#endif
        out.write(reinterpret_cast<const char*>(&value),24);uint32_t changed=std::memcmp(&value,&before,24)!=0;out.write(reinterpret_cast<const char*>(&changed),4);++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("Component framing");out.flush();if(!out)throw std::runtime_error("Complete component output");
#ifdef SFM64_NATIVE_PROXY_COMPONENT
    _mm_setcsr(mx);image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS component_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
