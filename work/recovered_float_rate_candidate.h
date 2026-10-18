#pragma once
#include <immintrin.h>
#include <cmath>
#include <cstdint>
#include <stdexcept>
inline uint32_t recoveredFloatRateCandidate(float fps){
 if(!std::isfinite(fps)||fps<0||fps>65535||(_mm_getcsr()&_MM_ROUND_MASK)!=_MM_ROUND_NEAREST)throw std::runtime_error("unsupported float frame rate or rounding mode");
 auto input=_mm_set_ss(fps);auto integer=_mm_cvtss_si32(input);
 auto difference=_mm_and_ps(_mm_sub_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),integer),input),_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
 if(_mm_comigt_ss(_mm_set_ss(0.001f),difference))return (uint32_t(integer)&65535u)|(10000u<<16);
 auto adjusted=_mm_mul_ss(_mm_mul_ss(input,_mm_set_ss(1001.f)),_mm_set_ss(0.001f));
 auto ntsc=_mm_cvtss_si32(adjusted);
 difference=_mm_and_ps(_mm_sub_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),ntsc),adjusted),_mm_castsi128_ps(_mm_set1_epi32(0x7fffffff)));
 if(_mm_comigt_ss(_mm_set_ss(0.01f),difference))return (uint32_t(ntsc)&65535u)|(10010u<<16);
 return (uint32_t(integer)&65535u)|(10000u<<16);
}
