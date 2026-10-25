#include "shader_skin_material_predicate_candidate.h"
#include "material_var_defined_candidate.h"
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <iterator>
#ifdef SFM64_NATIVE_SKIN_PREDICATE
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static uint32_t bumpCalls=0;
static uint8_t __fastcall isDefined(void*self,void*){++bumpCalls;return materialVarDefinedTypeCandidate(static_cast<uint8_t*>(self)[0x1c]);}
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("stdshader query fresh-output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("skin predicate IO");
#ifdef SFM64_NATIVE_SKIN_PREDICATE
    static_assert(sizeof(void*)==4,"private x86");NativePrivateWorldImageFixture image(argv[1],0xf0000);
    std::array<uint32_t,16>cvar{};const uintptr_t cvarPointer=reinterpret_cast<uintptr_t>(cvar.data());std::memcpy(image.base+0xcaff4,&cvarPointer,4);image.seal();
    std::array<uintptr_t,18>table{};table.fill(reinterpret_cast<uintptr_t>(&isDefined));
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    std::array<uint32_t,6>q{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),24)){
        if(q[0]>15||q[3]>255||q[5]>1)throw std::runtime_error("skin predicate query domain");int32_t global=0,phong=0,mask=0;std::memcpy(&global,&q[1],4);std::memcpy(&phong,&q[2],4);std::memcpy(&mask,&q[4],4);
        ShaderSkinMaterialInputsCandidate input;input.matPhong=global;if(q[0]&1)input.phong=phong;if(q[0]&2)input.diffuseWarpTypeByte=uint8_t(q[3]);if(q[0]&4)input.baseAlphaPhongMask=mask;if(q[0]&8)input.bumpDefined=q[5]!=0;
        const auto expected=shaderSkinMaterialPredicateCandidate(input);std::array<uint32_t,2>result{expected.wantsSkin?1u:0u,expected.bumpQueried?1u:0u};
#ifdef SFM64_NATIVE_SKIN_PREDICATE
        std::array<uint8_t,0xb0>info{};info.fill(255);std::array<std::array<uint8_t,32>,4>variables{};std::array<void*,4>params{};
        const std::array<size_t,4>fields{0xa8,0x8c,0xac,0x3c};
        for(uint32_t i=0;i<4;++i){const auto pointer=reinterpret_cast<uintptr_t>(table.data());std::memcpy(variables[i].data(),&pointer,4);if(q[0]&(1u<<i)){std::memcpy(info.data()+fields[i],&i,4);params[i]=variables[i].data();}}
        std::memcpy(variables[0].data()+8,&phong,4);variables[1][0x1c]=uint8_t(q[3]);std::memcpy(variables[2].data()+8,&mask,4);variables[3][0x1c]=q[5]?0:6;std::memcpy(&cvar[12],&global,4);
        const auto beforeInfo=info;const auto beforeVars=variables;const auto beforeCvar=cvar;bumpCalls=0;
        using Fn=uint8_t(__fastcall*)(void*,void*);const auto wants=reinterpret_cast<Fn>(image.base+0x54420)(info.data(),params.data());result={wants?1u:0u,bumpCalls};
        if(info!=beforeInfo||variables!=beforeVars||cvar!=beforeCvar)throw std::runtime_error("native predicate input mutation");
#endif
        out.write(reinterpret_cast<const char*>(result.data()),8);++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("skin predicate framing");out.flush();if(!out)throw std::runtime_error("predicate output");
#ifdef SFM64_NATIVE_SKIN_PREDICATE
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS skin_predicate_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
