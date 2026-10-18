#include <iostream>
#include "studio_material_vertex_layout_candidate.h"
int main(){
 const uintptr_t base=uintptr_t(0x123456789000ull),fallback=uintptr_t(0xabcdef012000ull);
 auto p=studioMaterialVertexLayoutCandidate(base,1,fallback);
 if(p.stride!=12||p.pointers[0]!=base||p.strides[0]!=12)return 1;
 // Position, normal, color, two-dimensional texture: offsets0/12/24/28,
 // forty-eight-byte padded stride versus thirty-six-byte unpadded stride.
 uint64_t format=1|2|4|(uint64_t(2)<<18);
 auto full=studioMaterialVertexLayoutCandidate(base,format,fallback);
 if(full.stride!=48||full.pointers[3]!=base+12||full.pointers[4]!=base+24||full.pointers[6]!=base+28)return 2;
 if(studioMaterialVertexLayoutCandidate(base,format|0x200,fallback).stride!=36)return 3;
 unsigned cases=0;
 for(unsigned flags=0;flags<4096;++flags)for(unsigned dimension=0;dimension<=4;++dimension){
  auto f=uint64_t(flags)|(uint64_t(dimension)<<18)|(uint64_t(3)<<12)|(uint64_t(2)<<15);
  auto d=studioMaterialVertexLayoutCandidate(base,f,fallback);
  for(unsigned i=0;i<18;++i){if(d.strides[i]&&d.strides[i]!=d.stride)return 4;if(d.pointers[i]!=fallback&&(d.pointers[i]<base||d.pointers[i]>base+d.stride))return 5;}
  if(d.compression!=unsigned((flags&0x400)!=0)||d.boneWeightCount!=3)return 6;
  ++cases;
 }
 unsigned rejected=0;
 for(unsigned channel=0;channel<8;++channel)for(unsigned dimension=5;dimension<=7;++dimension){try{studioMaterialVertexLayoutCandidate(base,uint64_t(dimension)<<(18+3*channel),fallback);return 7;}catch(const std::invalid_argument&){++rejected;}}
 std::cout<<"layout_cases="<<cases<<" known_offsets=1 full_width_pointers=1 invalid_texture_dimensions="<<rejected<<" native_oracle=0\n";
}
