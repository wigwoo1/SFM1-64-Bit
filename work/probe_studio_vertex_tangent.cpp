#include "studio_vertex_tangent_candidate.h"
#include "studio_vertex_weights_candidate.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <iterator>
#ifdef SFM64_NATIVE_VERTEX_TANGENT
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#endif
int main(int argc,char**argv){try{
    if(argc!=4||std::ifstream(argv[3],std::ios::binary).good())throw std::runtime_error("fresh tangent output required");
    std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("tangent fixture IO");
#ifdef SFM64_NATIVE_VERTEX_TANGENT
    static_assert(sizeof(void*)==4,"private x86");NativePrivateWorldImageFixture image(argv[1]);image.seal();
#else
    static_assert(sizeof(void*)==8,"owned x64");
#endif
    std::array<uint8_t,84>q{};size_t cases=0;
    while(in.read(reinterpret_cast<char*>(q.data()),84)){
        const bool compressed=q[57]!=0,hasTangent=q[59]!=0;const unsigned stored=q[58];if(q[57]>1||q[59]>1)throw std::runtime_error("tangent options");
        std::array<uint8_t,48>vertex{};std::array<uint8_t,9>opt{};std::array<uint8_t,8>initial{};std::array<float,3>normal{};std::array<float,4>tangent{};
        std::memcpy(vertex.data(),q.data(),48);std::memcpy(opt.data(),q.data()+48,9);std::memcpy(initial.data(),q.data()+60,8);std::memcpy(normal.data(),vertex.data()+28,12);std::memcpy(tangent.data(),q.data()+68,16);
        const auto weights=studioVertexWeightsCandidate(vertex,opt,compressed,stored,initial);
        // Admission only; native builds below overwrite every observed output
        // from actual unpatched writer execution, never candidate injection.
        const auto normalPacked=genericPackNormalCandidate(normal);
        StudioVertexTangentCandidate frame;if(hasTangent)frame=studioVertexTangentCandidate(normal,tangent,compressed);
        std::array<uint8_t,48>record{};std::memcpy(record.data(),weights.weights.data(),8);std::memcpy(record.data()+8,weights.indices.data(),4);record[12]=weights.valid?1:0;
        std::memset(record.data()+16,0xa5,28);if(compressed)std::memcpy(record.data()+16,hasTangent?frame.packedFrame.data():normalPacked.data(),4);else std::memcpy(record.data()+16,normal.data(),12);
        if(!compressed&&hasTangent)std::memcpy(record.data()+28,frame.tangent.data(),16);uint32_t advanced=1;std::memcpy(record.data()+44,&advanced,4);
#ifdef SFM64_NATIVE_VERTEX_TANGENT
        std::array<uint8_t,0x400>builder{};std::array<uint8_t,80>mesh{},header{};
        auto word=[](auto&b,size_t at,uint32_t v){std::memcpy(b.data()+at,&v,4);};
        auto pointer=[&](auto&b,size_t at,const void*p){word(b,at,uint32_t(reinterpret_cast<uintptr_t>(p)));};
        uint16_t original=0;std::memcpy(&original,opt.data()+4,2);
        const size_t vp=cases%3,tp=(cases+1)%4,mp=(cases+2)%3,vi=vp+mp+original,ti=tp+mp+original;
        std::vector<uint8_t>vertices((vi+1)*48,0xa5),tangents((ti+1)*16,0xa5);
        std::memcpy(vertices.data()+vi*48,vertex.data(),48);std::memcpy(tangents.data()+ti*16,tangent.data(),16);
        const auto beforeVertices=vertices,beforeTangents=tangents;
        word(header,0,uint32_t(vp*48));word(header,4,uint32_t(tp*16));word(mesh,4,uint32_t(mp));
        pointer(header,24,vertices.data());pointer(header,28,hasTangent?tangents.data():nullptr);pointer(mesh,40,header.data()+24);
        std::array<uint8_t,16>weightOut{},indexOut{};std::array<uint8_t,24>normalOut{},tangentOut{};
        weightOut.fill(0xa5);indexOut.fill(0xa5);normalOut.fill(0xa5);tangentOut.fill(0xa5);std::memcpy(weightOut.data()+4,initial.data(),8);
        std::array<uint8_t,32>position{},color{},uv{};
        word(builder,0xf4,12);word(builder,0xf8,compressed?4:8);word(builder,0xfc,4);word(builder,0x100,compressed?4:12);word(builder,0x104,4);word(builder,0x10c,8);word(builder,0x138,16);word(builder,0x144,stored);
        pointer(builder,0x14c,weightOut.data()+4);pointer(builder,0x150,indexOut.data()+4);pointer(builder,0x18c,tangentOut.data()+4);
        pointer(builder,0x1ac,position.data());pointer(builder,0x1b0,normalOut.data()+4);pointer(builder,0x1b4,color.data());pointer(builder,0x1b8,uv.data());
        using Fn=uint8_t(__stdcall*)(void*,void*,void*,void*,void*,void*,unsigned);
        const auto valid=reinterpret_cast<Fn>(image.base+(compressed?0x55aa0:0x560c0))(nullptr,nullptr,builder.data(),opt.data(),nullptr,mesh.data()+40,1);
        if(vertices!=beforeVertices||tangents!=beforeTangents||std::memcmp(vertex.data(),q.data(),48)||std::memcmp(opt.data(),q.data()+48,9))throw std::runtime_error("native source mutation");
        std::memcpy(record.data(),weightOut.data()+4,8);std::memcpy(record.data()+8,indexOut.data()+4,4);record[12]=valid?1:0;std::memcpy(record.data()+16,normalOut.data()+4,12);std::memcpy(record.data()+28,tangentOut.data()+4,16);std::memcpy(record.data()+44,builder.data()+0x1a8,4);
        for(unsigned i=0;i<24;++i){if((i<4||i>=16)&&normalOut[i]!=0xa5)throw std::runtime_error("native normal canary");if((i<4||i>=20)&&tangentOut[i]!=0xa5)throw std::runtime_error("native tangent canary");}
        for(unsigned i=0;i<16;++i){if((i<4||i>=12)&&weightOut[i]!=0xa5)throw std::runtime_error("native weight canary");if((i<4||i>=8)&&indexOut[i]!=0xa5)throw std::runtime_error("native index canary");}
#endif
        out.write(reinterpret_cast<const char*>(record.data()),48);++cases;
    }
    if(!in.eof()||in.gcount()||!cases)throw std::runtime_error("tangent query framing");out.flush();if(!out)throw std::runtime_error("tangent output failure");
#ifdef SFM64_NATIVE_VERTEX_TANGENT
    image.verify();VirtualFree(image.base,0,MEM_RELEASE);
#endif
    std::cout<<"PASS vertex_tangent_cases="<<cases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
