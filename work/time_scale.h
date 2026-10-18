#pragma once
#include <cstdint>
#include <xmmintrin.h>
#include <emmintrin.h>
// Reconstructs ifm.dll 0x1097ADB0's SSE sequence under the current MXCSR.
// No assumption about the tick-to-seconds scale is made.
inline int32_t scaleTicks(int32_t ticks,float scale){
 auto x=_mm_cvtepi32_ps(_mm_cvtsi32_si128(ticks));
 x=_mm_mul_ss(x,_mm_set_ss(scale));x=_mm_add_ss(x,_mm_set_ss(0.5f));
 auto sign=_mm_and_ps(_mm_castsi128_ps(_mm_set1_epi32(static_cast<int32_t>(0x80000000u))),x);
 auto magnitude=_mm_xor_ps(x,sign);
 auto mask=_mm_cmplt_ss(magnitude,_mm_set_ss(8388608.0f));
 auto bias=_mm_or_ps(_mm_and_ps(_mm_set_ss(8388608.0f),mask),sign);
 auto rounded=_mm_sub_ss(_mm_add_ss(x,bias),bias);
 auto correction=_mm_and_ps(_mm_cmpnle_ss(_mm_sub_ss(rounded,x),sign),_mm_set_ss(1.0f));
 return _mm_cvttss_si32(_mm_sub_ss(rounded,correction));
}
