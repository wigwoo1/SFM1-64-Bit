#pragma once
#include <immintrin.h>
// Recovered10498a60. Caller MXCSR applies; use /fp:strict for instruction-order
// validation. Inputs fully loaded before outputs, allowing input/output aliasing.
inline void concatenateControlLockAffineCandidate(const float* a,const float* b,float* out){
 const __m128 ar[3]={_mm_loadu_ps(a),_mm_loadu_ps(a+4),_mm_loadu_ps(a+8)};
 const __m128 br[3]={_mm_loadu_ps(b),_mm_loadu_ps(b+4),_mm_loadu_ps(b+8)};
 const __m128 mask=_mm_castsi128_ps(_mm_set_epi32(-1,0,0,0));
 for(int row=0;row<3;++row){
  auto value=_mm_mul_ps(_mm_shuffle_ps(ar[row],ar[row],0xaa),br[2]);
  value=_mm_add_ps(value,_mm_mul_ps(_mm_shuffle_ps(ar[row],ar[row],0x55),br[1]));
  value=_mm_add_ps(value,_mm_mul_ps(_mm_shuffle_ps(ar[row],ar[row],0),br[0]));
  value=_mm_add_ps(value,_mm_and_ps(ar[row],mask));
  _mm_storeu_ps(out+row*4,value);
 }
}
