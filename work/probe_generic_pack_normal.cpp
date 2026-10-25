#include "generic_packed_normal_candidate.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <iterator>
#ifdef SFM64_NATIVE_PACK_NORMAL
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("studio query fresh-output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("pack normal IO");
#ifdef SFM64_NATIVE_PACK_NORMAL
    static_assert(sizeof(void*)==4,"private x86");NativePrivateWorldImageFixture image(argv[1]);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    std::array<float,3>normal{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(normal.data()),12)){
        auto packed=genericPackNormalCandidate(normal);const auto before=normal;
#ifdef SFM64_NATIVE_PACK_NORMAL
        std::array<uint8_t,0xc0>builder{};std::array<uint8_t,12>buffer{};buffer.fill(0xa5);const auto pointer=reinterpret_cast<uintptr_t>(buffer.data()+4);std::memcpy(builder.data()+0xbc,&pointer,4);
        using Fn=void(__thiscall*)(void*,float,float,float);reinterpret_cast<Fn>(image.base+0xa100)(builder.data(),normal[0],normal[1],normal[2]);
        for(size_t i=0;i<4;++i)if(buffer[i]!=0xa5||buffer[i+8]!=0xa5)throw std::runtime_error("native pack canary");std::memcpy(packed.data(),buffer.data()+4,4);
#endif
        if(std::memcmp(before.data(),normal.data(),12))throw std::runtime_error("pack input mutation");out.write(reinterpret_cast<const char*>(packed.data()),4);++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("pack normal framing");out.flush();if(!out)throw std::runtime_error("pack output");
#ifdef SFM64_NATIVE_PACK_NORMAL
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS pack_normal_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
