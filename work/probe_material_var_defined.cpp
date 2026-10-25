#include "material_var_defined_candidate.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#ifdef SFM64_NATIVE_MATERIAL_VAR_DEFINED
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("private-library query fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);
    if(!in||!out)throw std::runtime_error("defined fixture IO");
#ifdef SFM64_NATIVE_MATERIAL_VAR_DEFINED
    static_assert(sizeof(void*)==4,"reference only");NativePrivateWorldImageFixture image(argv[1],0x100000);image.seal();
    uintptr_t method=0;std::memcpy(&method,image.base+0x98f34+17*4,4);
    if(method!=reinterpret_cast<uintptr_t>(image.base+0x18450))throw std::runtime_error("installed material variable slot17");
#else
    static_assert(sizeof(void*)==8,"owned x64 required");
#endif
    uint8_t type=0;size_t cases=0;
    while(in.read(reinterpret_cast<char*>(&type),1)){
        uint32_t result=materialVarDefinedTypeCandidate(type)?1u:0u;
#ifdef SFM64_NATIVE_MATERIAL_VAR_DEFINED
        std::array<uint8_t,40>object{};object.fill(0xa5);const auto table=reinterpret_cast<uintptr_t>(image.base+0x98f34);
        std::memcpy(object.data(),&table,4);object[0x1c]=type;const auto before=object;
        using Fn=uint8_t(__thiscall*)(void*);result=reinterpret_cast<Fn>(method)(object.data())?1u:0u;
        if(object!=before)throw std::runtime_error("IsDefined modified input object");
#endif
        out.write(reinterpret_cast<const char*>(&result),4);++cases;
    }
    if(!in.eof()||!cases)throw std::runtime_error("defined fixture framing");out.flush();if(!out)throw std::runtime_error("defined output");
#ifdef SFM64_NATIVE_MATERIAL_VAR_DEFINED
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS material_defined_types="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
