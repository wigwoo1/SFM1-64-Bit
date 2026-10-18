#pragma once
#include <cstring>
#include <cstdint>
#include <utility>
#include <emmintrin.h>
// Recovered engine.dll 1024A580. Requires strict FP compilation and the
// caller's SSE environment. Native parity: 131072 finite/singular/threshold
// states with separate and aliased output; exceptional environments pending.
inline bool engineProjectionInverseCandidate(const float* input,float* output){
 float rows[4][8]{}; int order[4]={0,1,2,3};
 for(int r=0;r<4;++r){std::memcpy(rows[r],input+r*4,16);rows[r][r+4]=1.0f;}
 const __m128 absMask=_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
 const __m128 signMask=_mm_castsi128_ps(_mm_set1_epi32(int(0x80000000u)));
 uint32_t thresholdBits=0x358637bd;float threshold;
 std::memcpy(&threshold,&thresholdBits,4);
 for(int c=0;c<4;++c){
  float best=threshold;int selected=-1;
  for(int r=c;r<4;++r){
   const float magnitude=_mm_cvtss_f32(_mm_and_ps(_mm_set_ss(rows[order[r]][c]),absMask));
   if(magnitude>best){best=magnitude;selected=r;}
  }
  if(selected<0)return false;
  std::swap(order[c],order[selected]);auto* pivot=rows[order[c]];
  const __m128 reciprocal=_mm_div_ss(_mm_set_ss(1.0f),_mm_set_ss(pivot[c]));
  for(int k=0;k<8;++k)pivot[k]=_mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(pivot[k]),reciprocal));
  pivot[c]=1.0f;
  for(int r=0;r<4;++r)if(r!=c){
   auto* row=rows[order[r]];
   const __m128 factor=_mm_xor_ps(_mm_set_ss(row[c]),signMask);
   for(int k=0;k<8;++k){
    const __m128 product=_mm_mul_ss(factor,_mm_set_ss(pivot[k]));
    row[k]=_mm_cvtss_f32(_mm_add_ss(product,_mm_set_ss(row[k])));
   }
   row[c]=0.0f;
  }
 }
 for(int r=0;r<4;++r)std::memcpy(output+r*4,rows[order[r]]+4,16);
 return true;
}
