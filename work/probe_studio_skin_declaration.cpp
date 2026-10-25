#include "studio_skin_declaration_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstring>
#ifdef SFM64_NATIVE_SKIN_DECL
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static int declarationLevel=95;
static int __fastcall hardwareQuery(void*,void*){return declarationLevel;}
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("shaderapi query fresh-output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("declaration IO");
#ifdef SFM64_NATIVE_SKIN_DECL
    static_assert(sizeof(void*)==4,"private x86");NativePrivateWorldImageFixture image(argv[1],0x1c6000);
    std::array<uintptr_t,18>table{};table.fill(reinterpret_cast<uintptr_t>(&hardwareQuery));uintptr_t hardware=reinterpret_cast<uintptr_t>(table.data());auto ptr=reinterpret_cast<uintptr_t>(&hardware);std::memcpy(image.base+0x19092c,&ptr,4);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    std::array<uint32_t,4>q{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),16)){
        int32_t level=0;std::memcpy(&level,&q[2],4);if(q[3]>1)throw std::runtime_error("declaration flex flag");const auto expected=studioSkinDeclarationCandidate(uint64_t(q[0])|(uint64_t(q[1])<<32),level,q[3]!=0);auto result=expected;
#ifdef SFM64_NATIVE_SKIN_DECL
        std::array<StudioSkinDeclarationElementCandidate,35>buffer{};for(auto&x:buffer)x.fill(0xa5);declarationLevel=level;
        using Fn=void(__cdecl*)(uint32_t,uint32_t,void*,unsigned,unsigned,unsigned,unsigned,void*);
        reinterpret_cast<Fn>(image.base+0x31660)(q[0],q[1],buffer.data()+1,0,q[3],0,0,nullptr);
        size_t end=1;while(end<34&&!(buffer[end][0]==255&&buffer[end][1]==0))++end;if(end>=34)throw std::runtime_error("native declaration END");
        for(size_t i=0;i<8;++i)if(buffer.front()[i]!=0xa5)throw std::runtime_error("native declaration leading canary");
        for(size_t i=end+1;i<buffer.size();++i)for(uint8_t v:buffer[i])if(v!=0xa5)throw std::runtime_error("native declaration trailing canary");
        result.assign(buffer.begin()+1,buffer.begin()+end+1);
#endif
        const uint32_t n=uint32_t(result.size());out.write(reinterpret_cast<const char*>(&n),4);out.write(reinterpret_cast<const char*>(result.data()),n*8);++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("declaration framing");out.flush();if(!out)throw std::runtime_error("declaration output");
#ifdef SFM64_NATIVE_SKIN_DECL
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS declaration_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
