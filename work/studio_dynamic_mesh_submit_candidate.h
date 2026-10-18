#pragma once
#include <array>
#include <cstring>
#include <immintrin.h>
#include "studio_shader_instance_state_candidate.h"
// Logical adapter for100093f0. Submit must consume the stack record synchronously.
// The receiver, matrices and lighting retain full-width pointers; no native ABI.
template<class Ops>void studioDynamicMeshSubmitCandidate(uintptr_t mesh,const std::array<float,4>* modulation,Ops& ops){
 auto result=ops.instance();
 ops.offsets(mesh,result.instance.scalars[11],result.instance.scalars[12]);
 std::array<float,4> rgba;
 if(modulation)rgba=*modulation; // Capture before material callbacks can mutate it.
 auto material=ops.material();
 std::array<float,3> rgb;ops.rgb(material,rgb);
 float alpha=ops.alpha(material);
 std::array<float,4> materialColor{rgb[0],rgb[1],rgb[2],alpha};
 for(unsigned i=0;i<4;++i){
  float value=materialColor[i];
  if(modulation)value=_mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(value),_mm_set_ss(rgba[i])));
  std::memcpy(&result.instance.scalars[15+i],&value,sizeof(value));
 }
 auto count=ops.count(mesh);ops.setCount(count);
 auto format=ops.format(mesh);
 ops.submit(mesh,uint32_t(1),result.instance,(format[0]&0x400u)!=0,result.metadata1,result.metadata2);
}
