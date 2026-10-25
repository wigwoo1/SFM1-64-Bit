#include "ifm_bone_world_palette_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef SFM64_NATIVE_BONE_PALETTE
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
static std::array<uint8_t,0x110>model{};
static void* __fastcall resolve(void*,void*,uint32_t){return model.data();}
static bool __fastcall isGame(void*,void*,uint32_t){return false;} // no precomputed DAG bones
#endif
int main(int argc,char**argv){try{
#ifdef SFM64_NATIVE_BONE_PALETTE
    static_assert(sizeof(void*)==4,"x86 oracle only");
    if(argc!=4)throw std::runtime_error("original.dll queries.bin fresh-output.bin required");
    NativePrivateWorldImageFixture image(argv[1]);
    std::array<uintptr_t,19>dmTable{};dmTable[18]=reinterpret_cast<uintptr_t>(&resolve);
    uintptr_t dmObject=reinterpret_cast<uintptr_t>(dmTable.data());*reinterpret_cast<uintptr_t*>(image.base+0xe9a51c)=reinterpret_cast<uintptr_t>(&dmObject);
    std::array<uintptr_t,5>typeTable{};typeTable[4]=reinterpret_cast<uintptr_t>(&isGame);auto type=reinterpret_cast<uintptr_t>(typeTable.data());std::memcpy(model.data(),&type,4);
    // model+104 remains0: procedural evaluation off. Non-INT_MIN timestamp
    // bypasses clock lookup. No code patches; complete original kernel runs.
    image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64 required");
    if(argc!=4)throw std::runtime_error("ignored queries.bin fresh-output.bin required");
#endif
    if(std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("fresh output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("palette IO");
    std::array<uint32_t,2>header{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(header.data()),8)){
        if(header[0]>256)throw std::runtime_error("fixture palette count");const size_t count=header[0];
        RecoveredMatrix3x4 root{};in.read(reinterpret_cast<char*>(root.data()),48);
        std::vector<IfmBoneLocalPoseCandidate>bones(count);
        static_assert(sizeof(IfmBoneLocalPoseCandidate)==40,"wire semantic pose");
        in.read(reinterpret_cast<char*>(bones.data()),std::streamsize(count*40));if(!in)throw std::runtime_error("truncated palette fixture");
        // Validate the supported snapshot before invoking original code.
        const auto expected=ifmBoneWorldPaletteCandidate(bones,root,header[1]);
        std::vector<RecoveredMatrix3x4>palette(count);for(auto&m:palette)for(auto&r:m)r.fill(12345.25f);
#ifdef SFM64_NATIVE_BONE_PALETTE
        std::vector<uint8_t>studio(0x200+count*216);uint32_t n=uint32_t(count),offset=0x200;
        std::memcpy(studio.data()+0x9c,&n,4);std::memcpy(studio.data()+0xa0,&offset,4);
        for(size_t i=0;i<count;++i){std::memcpy(studio.data()+0x200+i*216+4,&bones[i].parent,4);std::memcpy(studio.data()+0x200+i*216+160,&bones[i].flags,4);}
        uintptr_t wrapper=reinterpret_cast<uintptr_t>(studio.data());std::array<uintptr_t,2>paletteWrapper{0,reinterpret_cast<uintptr_t>(palette.data())};
        std::array<uint8_t,0x80>object{};auto w=reinterpret_cast<uintptr_t>(&wrapper),p=reinterpret_cast<uintptr_t>(paletteWrapper.data());std::memcpy(object.data()+0x70,&w,4);std::memcpy(object.data()+0x18,&p,4);
        std::vector<std::array<float,3>>positions;std::vector<std::array<float,4>>orientations;std::vector<float>scales;
        for(const auto&b:bones){positions.push_back(b.position);orientations.push_back(b.orientation);scales.push_back(b.scale);}
        std::array<uint32_t,8>computed{};const auto beforeRoot=root;const auto beforeBones=bones;const auto beforeStudio=studio;
        using Kernel=void(__thiscall*)(void*,const void*,const void*,const float*,const void*,uint32_t,void*,int32_t);
        reinterpret_cast<Kernel>(image.base+0x2411b0)(object.data(),positions.data(),orientations.data(),scales.data(),root.data(),header[1],computed.data(),0);
        if(root!=beforeRoot||std::memcmp(bones.data(),beforeBones.data(),count*40)||studio!=beforeStudio)throw std::runtime_error("palette input mutation");
#else
        for(size_t i=0;i<count;++i)if(expected[i])palette[i]=*expected[i];
#endif
        out.write(reinterpret_cast<const char*>(palette.data()),std::streamsize(count*48));++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("palette fixture shape");out.flush();if(!out)throw std::runtime_error("palette output");
#ifdef SFM64_NATIVE_BONE_PALETTE
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS bone_world_cases="<<cases<<" procedural=disabled precomputed_bones=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
