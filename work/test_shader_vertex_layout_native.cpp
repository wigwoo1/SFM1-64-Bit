#define NOMINMAX
#include <windows.h>
#include <cstring>
#include <iostream>
#include "studio_material_vertex_layout_candidate.h"
#ifndef _WIN64
#include "shader_vertex_layout_native_init.h"
#endif
int main(){
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,12288,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!code)return 1;
#ifndef _WIN64
 initializeLayout(code);DWORD old;if(!VirtualProtect(code,8192,PAGE_EXECUTE_READ,&old))return 2;
 FlushInstructionCache(GetCurrentProcess(),code,8192);
 using Function=void(__cdecl*)(uint32_t,uint32_t,uint32_t,void*);auto native=reinterpret_cast<Function>(code);
#endif
#ifdef _WIN64
 uintptr_t base=0x123456789000ull;
#else
 uintptr_t base=0x34560000;
#endif
 uintptr_t fallback=reinterpret_cast<uintptr_t>(code+8208);unsigned cases=0;uint64_t digest=1469598103934665603ull;
 auto mix=[&](uint32_t word){for(unsigned i=0;i<4;++i){digest^=(word>>(i*8))&255;digest*=1099511628211ull;}};
 auto check=[&](uint64_t format)->int{
 uint32_t words[46];for(auto& word:words)word=0xa5a5a5a5;
#ifndef _WIN64
 native(uint32_t(base),uint32_t(format),uint32_t(format>>32),words);
#endif
 auto d=studioMaterialVertexLayoutCandidate(base,format,fallback);
#ifndef _WIN64
 for(unsigned i=0;i<18;++i)if(words[i]!=d.strides[i]||words[21+i]!=d.pointers[i]){std::cout<<"mismatch format="<<format<<" field="<<i<<" native_stride="<<words[i]<<" candidate_stride="<<d.strides[i]<<" native_pointer="<<words[21+i]<<" candidate_pointer="<<d.pointers[i]<<"\n";return 3;}
 if(words[18]!=d.stride||words[19]!=d.compression||words[20]!=d.boneWeightCount)return 4;
 for(unsigned i=39;i<46;++i)if(words[i]!=0xa5a5a5a5)return 5;
#endif
 for(unsigned i=0;i<18;++i){mix(d.strides[i]);mix(d.pointers[i]==fallback?0xffffffff:uint32_t(d.pointers[i]-base));}mix(d.stride);mix(d.compression);mix(d.boneWeightCount);
 ++cases;return 0;
 };
 for(unsigned flags=0;flags<4096;++flags)for(unsigned weights=0;weights<8;++weights)for(unsigned channel=0;channel<8;++channel)for(unsigned dim=0;dim<=4;++dim){
 uint64_t format=uint64_t(flags)|(uint64_t(weights)<<12)|(uint64_t(channel%8)<<15)|(uint64_t(dim)<<(18+3*channel));
 if(auto failure=check(format))return failure;
 }
 unsigned multiCases=0;
 // Exhaust every valid dimension tuple for all eight channels, with all
 // compression/position-normal variant/padding combinations and other fields.
 for(unsigned tuple=0;tuple<390625;++tuple){
  unsigned remaining=tuple;uint64_t textures=0;
  for(unsigned channel=0;channel<8;++channel){textures|=uint64_t(remaining%5)<<(18+3*channel);remaining/=5;}
  for(unsigned mode=0;mode<8;++mode){
   uint64_t format=textures|0xff|(uint64_t(tuple%8)<<12)|(uint64_t((tuple/8)%8)<<15);
   if(mode&1)format|=0x400;if(mode&2)format|=0x800;if(mode&4)format|=0x200;
   if(auto failure=check(format))return failure;++multiCases;
  }
 }

 VirtualFree(code,0,MEM_RELEASE);std::cout<<"layout_cases="<<cases<<" normalized_digest="<<digest<<" pointer_bits="<<sizeof(uintptr_t)*8<<" multichannel_cases="<<multiCases<<" descriptor_fields=39 texture_dimension_range=0..4\n";
}
