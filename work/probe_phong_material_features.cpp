// Complete private original 10037F40 vs owned native x64. No native imports,
// callbacks, material-system initialization, GPU or live SFM application access.
#define NOMINMAX
#include <windows.h>
#include "phong_material_features_candidate.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#ifndef _WIN64
#include "native_private_world_image_fixture.h"
struct NativeFeatureVar {std::array<uint8_t,32>bytes;};
__declspec(naked) static void __cdecl callOriginal(uint32_t,uint32_t*,NativeFeatureVar**,uint8_t*,uint32_t){
    __asm {
        push esi
        mov eax,[esp+8]
        mov ecx,[esp+12]
        mov edx,[esp+16]
        mov esi,[esp+20]
        push dword ptr [esp+24]
        call eax
        add esp,4
        pop esi
        ret
    }
}
#endif
struct Query {uint32_t serial,flags,rimGate,flashlightOnly;std::array<std::array<uint32_t,3>,13>values;};
static_assert(sizeof(Query)==172,"query ABI");
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3]).good())throw std::runtime_error("usage: stdshader query fresh-output");
    std::ifstream input(argv[2],std::ios::binary);if(!input)throw std::runtime_error("query file");
    std::ofstream output(argv[3],std::ios::binary);if(!output)throw std::runtime_error("output file");
#ifndef _WIN64
    NativePrivateWorldImageFixture image(argv[1],0x1000);
    std::array<uint8_t,0x60>control{};const uintptr_t controlPointer=reinterpret_cast<uintptr_t>(control.data());
    std::memcpy(image.base+0xc5e6c,&controlPointer,4);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64 required");(void)argv[1];
#endif
    Query q{};uint32_t cases=0;
    while(input.read(reinterpret_cast<char*>(&q),sizeof(q))){
        if(q.serial!=cases||q.flashlightOnly>255)throw std::runtime_error("query identity/boolean");
        PhongMaterialFeaturesInputsCandidate in;in.flags=q.flags;in.globalRimGate=q.rimGate;in.flashlightOnly=q.flashlightOnly!=0;
        for(size_t i=0;i<q.values.size();++i){const auto&v=q.values[i];if(v[0]>1||v[1]>255)throw std::runtime_error("typed variable framing");in.values[i]={v[0]!=0,uint8_t(v[1]),v[2]};}
        const auto saved=q;
        // Native-only validation is deliberately not the owned implementation.
        if(!q.flashlightOnly&&!q.values[6][0])throw std::runtime_error("EnvMap required index");
        std::array<uint8_t,11>result{};
#ifndef _WIN64
        std::array<NativeFeatureVar,14>vars{};std::array<NativeFeatureVar*,125>params{};
        std::array<uint32_t,110>info{};info.fill(0xffffffffu);
        const uint32_t indices[13]={6,14,15,26,27,28,20,34,48,44,43,17,58};
        const uint32_t offsets[13]={0,4,8,0x3c,0x40,0x44,0x24,0x70,0xa8,0x90,0x8c,0x18,0xc0};
        for(size_t i=0;i<vars.size();++i)vars[i].bytes.fill(0x7c);
        std::memcpy(vars[13].bytes.data()+8,&q.flags,4);params[0]=&vars[13];
        for(size_t i=0;i<13;++i){params[indices[i]]=&vars[i];vars[i].bytes[0x1c]=uint8_t(q.values[i][1]);std::memcpy(vars[i].bytes.data()+8,&q.values[i][2],4);
            if(q.values[i][0])info[offsets[i]/4]=indices[i];}
        control.fill(0xa5);std::memcpy(control.data()+0x30,&q.rimGate,4);
        const auto beforeVars=vars;const auto beforeParams=params;const auto beforeInfo=info;const auto beforeControl=control;
        std::array<uint8_t,32>guard{};guard.fill(0x9b);
        callOriginal(uint32_t(reinterpret_cast<uintptr_t>(image.base+0x37f40)),info.data(),params.data(),guard.data()+4,q.flashlightOnly);
        std::memcpy(result.data(),guard.data()+4,11);
        for(size_t i=0;i<guard.size();++i)if((i<4||i>=15)&&guard[i]!=0x9b)throw std::runtime_error("result guard mutation");
        if(std::memcmp(vars.data(),beforeVars.data(),sizeof(vars))||params!=beforeParams||info!=beforeInfo||control!=beforeControl)throw std::runtime_error("native input mutation");
#else
        result=phongMaterialFeaturesCandidate(in);
#endif
        if(std::memcmp(&q,&saved,sizeof(q)))throw std::runtime_error("query mutation");
        output.write(reinterpret_cast<const char*>(&q.serial),4);output.write(reinterpret_cast<const char*>(result.data()),11);++cases;
    }
    if(!input.eof()||input.gcount()||!cases)throw std::runtime_error("query framing");output.flush();if(!output)throw std::runtime_error("trace write");
#ifndef _WIN64
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS material_feature_cases="<<cases<<" original_complete_helper=1 gpu_calls=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
