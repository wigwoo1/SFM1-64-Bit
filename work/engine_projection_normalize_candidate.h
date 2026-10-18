#pragma once
#include <emmintrin.h>
#include <cstdint>
#include <cstring>
// engine.dll10244060: returns unadjusted length, writes epsilon-scaled vector.
// Requires caller SSE environment; original execution parity pending.
inline float engineProjectionNormalizeCandidate(float* vector){
 const __m128 x=_mm_set_ss(vector[0]),y=_mm_set_ss(vector[1]),z=_mm_set_ss(vector[2]);
 const __m128 squares=_mm_add_ss(_mm_add_ss(_mm_mul_ss(x,x),_mm_mul_ss(y,y)),_mm_mul_ss(z,z));
 const __m128 length=_mm_sqrt_ss(squares);
 uint32_t bits=0x34000000;float epsilon;std::memcpy(&epsilon,&bits,4);
 const __m128 scale=_mm_div_ss(_mm_set_ss(1.0f),_mm_add_ss(length,_mm_set_ss(epsilon)));
 vector[0]=_mm_cvtss_f32(_mm_mul_ss(x,scale));
 vector[1]=_mm_cvtss_f32(_mm_mul_ss(y,scale));
 vector[2]=_mm_cvtss_f32(_mm_mul_ss(z,scale));
 return _mm_cvtss_f32(length);
}
