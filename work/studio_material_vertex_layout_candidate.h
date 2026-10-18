#pragma once
#include <array>
#include <stdexcept>
#include "studio_material_vertex_element_size_candidate.h"
// Logical field ordering matches native descriptor strides0..44/pointers54..98.
// Native public ABI is not implied. Texture dimensions must be0..4; pointers
// and additions must be representable. No fallback buffer is dereferenced here.
struct StudioMaterialVertexLayoutCandidate {
 std::array<uint32_t,18> strides{};
 std::array<uintptr_t,18> pointers{};
 uint32_t stride=0,compression=0,boneWeightCount=0;
};
inline StudioMaterialVertexLayoutCandidate studioMaterialVertexLayoutCandidate(
 uintptr_t vertices,uint64_t format,uintptr_t fallback){
 for(unsigned i=0;i<8;++i)if(((format>>(18+3*i))&7)>4)
  throw std::invalid_argument("unsupported texture dimension");
 StudioMaterialVertexLayoutCandidate d;d.pointers.fill(fallback);
 d.compression=(format&0x400)!=0;d.boneWeightCount=uint32_t((format>>12)&7);
 std::array<bool,18> active{};uint32_t offset=0;
 auto add=[&](unsigned field,uint32_t bytes){d.pointers[field]=vertices+offset;active[field]=true;offset+=bytes;};
 auto size=[&](unsigned element){return studioMaterialVertexElementSizeCandidate(element,d.compression!=0);};
 if(format&1){add(0,size((format&0x800)?1:0));if(format&0x40)add(16,4);}
 if(format&0x80){if(d.boneWeightCount)add(1,d.compression?4:8);add(2,4);}
 if(format&2)add(3,size((format&0x800)?3:2));
 if(format&4)add(4,4);
 if(format&8)add(5,4);
 for(unsigned i=0;i<8;++i){unsigned dimension=unsigned((format>>(18+3*i))&7);if(dimension)add(6+i,size(18+(dimension-1)*8+i));}
 if(format&0x10)add(14,12);
 if(format&0x20)add(15,12);
 unsigned user=unsigned((format>>15)&7);if(user)add(17,size(13+user));
 if(!(format&0x200)&&offset>16)offset=(offset+15)&~uint32_t(15);
 d.stride=offset;
 for(unsigned i=0;i<18;++i)if(active[i])d.strides[i]=offset;
 return d;
}
