#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <stdexcept>
using StudioSkinDeclarationElementCandidate=std::array<uint8_t,8>;
// Bounded skinned stream0 + default flex stream2 declaration. No color stream1,
// instancing, vertex-ID stream, override streams or alternate float4 inputs.
// Reproduces builder bytes; does not certify a format for GPU admission.
inline std::vector<StudioSkinDeclarationElementCandidate>studioSkinDeclarationCandidate(uint64_t format,int level,bool uncompressedFlex){
    constexpr uint64_t allowed=0x6bfull|(7ull<<12)|(7ull<<15)|(((1ull<<24)-1)<<18);
    const auto weights=unsigned((format>>12)&7),user=unsigned((format>>15)&7);
    if((format&~allowed)||(format&0x83)!=0x83||weights<1||weights>2||(user!=0&&user!=4))throw std::invalid_argument("unsupported skin declaration format");
    for(unsigned i=0;i<8;++i)if(((format>>(18+3*i))&7)>4)throw std::invalid_argument("skin declaration texture dimension");
    const bool compressed=(format&0x400)!=0;std::vector<StudioSkinDeclarationElementCandidate>out;uint16_t offset=0,normalOffset=0;
    auto add=[&](uint16_t stream,uint16_t at,uint8_t type,uint8_t usage,uint8_t index=0){out.push_back({uint8_t(stream),uint8_t(stream>>8),uint8_t(at),uint8_t(at>>8),type,0,usage,index});};
    add(0,offset,2,0);offset+=12;
    add(0,offset,compressed?6:1,1);offset+=compressed?4:8;
    add(0,offset,4,2);offset+=4;
    normalOffset=offset;add(0,offset,compressed?5:2,3);offset+=compressed?4:12;
    if(format&4){add(0,offset,4,10);offset+=4;}
    if(format&8){add(0,offset,4,10,1);offset+=4;}
    for(unsigned i=0;i<8;++i){auto dimension=unsigned((format>>(18+3*i))&7);if(dimension){add(0,offset,uint8_t(dimension-1),5,uint8_t(i));offset+=uint16_t(dimension*4);}}
    if(format&0x10){add(0,offset,2,6);offset+=12;}
    if(format&0x20){add(0,offset,2,7);offset+=12;}
    if(user)add(0,compressed?normalOffset:offset,compressed?5:3,6);
    const bool newer=level>=92;add(2,0,newer?3:2,0,1);
    add(2,newer?16:12,compressed&&!uncompressedFlex?5:2,3,1);
    out.push_back({255,0,0,0,17,0,0,0});return out;
}
