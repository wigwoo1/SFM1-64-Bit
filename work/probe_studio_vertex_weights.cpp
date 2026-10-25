#include "studio_vertex_weights_candidate.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <iterator>
#ifdef SFM64_NATIVE_VERTEX_WEIGHTS
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("studio.dll query fresh-output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("vertex weight IO");
#ifdef SFM64_NATIVE_VERTEX_WEIGHTS
    static_assert(sizeof(void*)==4,"private x86");NativePrivateWorldImageFixture image(argv[1]);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    std::array<uint8_t,48>vertex{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(vertex.data()),48)){
        std::array<uint8_t,9>optimized{};std::array<uint8_t,3>options{};std::array<uint8_t,8>initial{};
        in.read(reinterpret_cast<char*>(optimized.data()),9);in.read(reinterpret_cast<char*>(options.data()),3);in.read(reinterpret_cast<char*>(initial.data()),8);if(!in||options[0]>1||options[2])throw std::runtime_error("vertex weight framing");
        const auto expected=studioVertexWeightsCandidate(vertex,optimized,options[0]!=0,options[1],initial);
        auto result=expected;const auto beforeVertex=vertex;const auto beforeOptimized=optimized;
#ifdef SFM64_NATIVE_VERTEX_WEIGHTS
        // Full original vertex writer, no code patches or imports. Optional
        // tangent input is absent; all destinations and builder are disposable.
        std::array<uint8_t,0x400>builder{};std::array<uint8_t,80>mesh{},header{};
        auto word=[](auto&b,size_t at,uint32_t v){std::memcpy(b.data()+at,&v,4);};
        auto pointer=[&](auto&b,size_t at,const void*p){word(b,at,uint32_t(reinterpret_cast<uintptr_t>(p)));};
        std::array<uint8_t,16>weights{},indices{};weights.fill(0xa5);indices.fill(0xa5);std::memcpy(weights.data()+4,initial.data(),8);
        std::array<uint8_t,32>position{},normal{},color{},uv{};
        uint16_t originalId=0;std::memcpy(&originalId,optimized.data()+4,2);
        std::vector<uint8_t>sourceVertices((size_t(originalId)+1)*48);
        std::memcpy(sourceVertices.data()+size_t(originalId)*48,vertex.data(),48);
        pointer(header,24,sourceVertices.data());pointer(mesh,40,header.data()+24);
        word(builder,0xf4,12);word(builder,0xf8,options[0]?4:8);word(builder,0xfc,4);word(builder,0x100,options[0]?4:12);word(builder,0x104,4);word(builder,0x10c,8);word(builder,0x144,options[1]);
        pointer(builder,0x14c,weights.data()+4);pointer(builder,0x150,indices.data()+4);
        pointer(builder,0x1ac,position.data());pointer(builder,0x1b0,normal.data());pointer(builder,0x1b4,color.data());pointer(builder,0x1b8,uv.data());
        using Fn=uint8_t(__stdcall*)(void*,void*,void*,void*,void*,void*,unsigned);
        const auto valid=reinterpret_cast<Fn>(image.base+(options[0]?0x55aa0:0x560c0))(nullptr,nullptr,builder.data(),optimized.data(),nullptr,mesh.data()+40,1);
        if(std::memcmp(sourceVertices.data()+size_t(originalId)*48,vertex.data(),48))throw std::runtime_error("native source vertex mutation");
        result.valid=valid!=0;std::memcpy(result.weights.data(),weights.data()+4,8);std::memcpy(result.indices.data(),indices.data()+4,4);
        for(size_t i:{size_t(0),size_t(1),size_t(2),size_t(3),size_t(12),size_t(13),size_t(14),size_t(15)})if(weights[i]!=0xa5||indices[i]!=0xa5)throw std::runtime_error("native writer output canary");
        for(size_t i=8;i<12;++i)if(indices[i]!=0xa5)throw std::runtime_error("native index output extent");
        uint32_t advanced=0;std::memcpy(&advanced,builder.data()+0x1a8,4);if(advanced!=1)throw std::runtime_error("native vertex advance");
#endif
        if(vertex!=beforeVertex||optimized!=beforeOptimized)throw std::runtime_error("native writer input mutation");
        std::array<uint8_t,16>record{};std::memcpy(record.data(),result.weights.data(),8);std::memcpy(record.data()+8,result.indices.data(),4);record[12]=result.valid?1:0;out.write(reinterpret_cast<const char*>(record.data()),16);++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("vertex weight framing/end");out.flush();if(!out)throw std::runtime_error("vertex weight output");
#ifdef SFM64_NATIVE_VERTEX_WEIGHTS
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS vertex_weight_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
