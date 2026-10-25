#include "studio_skin_palette_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#ifdef SFM64_NATIVE_STUDIO_SKIN
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("studio.dll query fresh-output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("studio skin IO");
#ifdef SFM64_NATIVE_STUDIO_SKIN
    static_assert(sizeof(void*)==4,"private x86");NativePrivateWorldImageFixture image(argv[1]);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    size_t cases=0;std::array<uint32_t,3>h{};
    while(in.read(reinterpret_cast<char*>(h.data()),12)){
        if(h[0]>0x1000000||!h[1]||h[1]>256)throw std::runtime_error("studio skin fixture limit");std::vector<uint8_t>mdl(h[0]);std::vector<RecoveredMatrix3x4>world(h[1]),output(h[1]);in.read(reinterpret_cast<char*>(mdl.data()),h[0]);in.read(reinterpret_cast<char*>(world.data()),h[1]*48);in.read(reinterpret_cast<char*>(output.data()),h[1]*48);if(!in)throw std::runtime_error("studio skin truncated");
        const auto metadata=readStudioSkinPaletteMetadataCandidate(mdl);const auto expected=studioSkinPaletteCandidate(metadata,world,h[2],output);const auto beforeMdl=mdl;const auto beforeWorld=world;
#ifdef SFM64_NATIVE_STUDIO_SKIN
        RecoveredMatrix3x4 guard{};for(auto&r:guard)r.fill(12345.25f);std::vector<RecoveredMatrix3x4>guarded(output.size()+2,guard);std::copy(output.begin(),output.end(),guarded.begin()+1);
        using Fn=void(__cdecl*)(void*,void*,uint32_t,void*,const void*);
        reinterpret_cast<Fn>(image.base+0x1d90)(guarded.data()+1,mdl.data(),h[2],nullptr,world.data());
        if(guarded.front()!=guard||guarded.back()!=guard)throw std::runtime_error("studio skin output canary");std::copy(guarded.begin()+1,guarded.end()-1,output.begin());
#else
        output=expected;
#endif
        if(mdl!=beforeMdl||std::memcmp(world.data(),beforeWorld.data(),world.size()*48))throw std::runtime_error("studio skin input mutation");out.write(reinterpret_cast<const char*>(output.data()),std::streamsize(output.size()*48));++cases;
    }if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("studio skin framing");out.flush();if(!out)throw std::runtime_error("studio skin output");
#ifdef SFM64_NATIVE_STUDIO_SKIN
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS studio_skin_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
