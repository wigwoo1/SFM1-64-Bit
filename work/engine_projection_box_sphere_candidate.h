#pragma once
#include <emmintrin.h>
inline bool engineProjectionBoxSphereCandidate(const float* minimum,const float* maximum,const float* center,float radius){
 __m128 distance=_mm_setzero_ps();
 for(int axis=0;axis<3;++axis){
  __m128 delta=_mm_setzero_ps();bool outside=true;
  if(minimum[axis]>center[axis])delta=_mm_sub_ss(_mm_set_ss(center[axis]),_mm_set_ss(minimum[axis]));
  else if(center[axis]>maximum[axis])delta=_mm_sub_ss(_mm_set_ss(maximum[axis]),_mm_set_ss(center[axis]));
  else outside=false;
  if(outside){const __m128 square=_mm_mul_ss(delta,delta);distance=axis==0?square:(axis==1?_mm_add_ss(square,distance):_mm_add_ss(distance,square));}
 }
 const __m128 r=_mm_set_ss(radius);return _mm_cvtss_f32(_mm_mul_ss(r,r))>_mm_cvtss_f32(distance);
}

