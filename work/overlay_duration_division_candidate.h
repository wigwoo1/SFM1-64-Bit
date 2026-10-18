#pragma once
#include <cstdint>
#include <xmmintrin.h>
#include <emmintrin.h>
// Instruction-order translation of1097ae30. Depends on current MXCSR just as
// original SSE path does; does not replace the sequence with std::round.
inline int32_t overlayDurationDivisionCandidate(int32_t& duration,float divisor){
 auto value=_mm_cvtepi32_ps(_mm_cvtsi32_si128(duration));
 auto signMask=_mm_castsi128_ps(_mm_set1_epi32(INT32_MIN));
 auto magic=_mm_set_ss(8388608.0f);
 auto one=_mm_set_ss(1.0f);
 value=_mm_div_ss(value,_mm_set_ss(divisor));
 value=_mm_add_ss(value,_mm_set_ss(0.5f));
 auto sign=_mm_and_ps(signMask,value);
 auto magnitude=_mm_xor_ps(value,sign);
 auto small=_mm_cmplt_ss(magnitude,magic);
 auto signedMagic=_mm_or_ps(_mm_and_ps(magic,small),sign);
 auto rounded=_mm_add_ss(value,signedMagic);
 rounded=_mm_sub_ss(rounded,signedMagic);
 auto delta=_mm_sub_ss(rounded,value);
 auto correction=_mm_and_ps(_mm_cmpnle_ss(delta,sign),one);
 rounded=_mm_sub_ss(rounded,correction);
 duration=_mm_cvttss_si32(rounded);
 return duration;
}
