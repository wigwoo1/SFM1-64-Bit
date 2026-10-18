#pragma once
#include <immintrin.h>
#include <cstdint>
#include <cstring>
// Original ifm1097B170, scalar SSE sequence under default FP controls.
inline int32_t engineSecondsTicksCandidate(uint32_t secondsWord){
 float seconds;std::memcpy(&seconds,&secondsWord,4);
 auto value=_mm_add_ss(_mm_mul_ss(_mm_set_ss(seconds),_mm_set_ss(10000.f)),_mm_set_ss(.5f));
 auto sign=_mm_and_ps(_mm_castsi128_ps(_mm_set1_epi32(int32_t(0x80000000u))),value);
 auto magnitude=_mm_xor_ps(value,sign);auto threshold=_mm_set_ss(8388608.f);
 auto bias=_mm_or_ps(_mm_and_ps(threshold,_mm_cmplt_ss(magnitude,threshold)),sign);
 auto rounded=_mm_sub_ss(_mm_add_ss(value,bias),bias);
 auto correction=_mm_and_ps(_mm_cmpnle_ss(_mm_sub_ss(rounded,value),sign),_mm_set_ss(1));
 return _mm_cvttss_si32(_mm_sub_ss(rounded,correction));
}
// Original ifm10275640 stores new current only when ordered greater.
inline int32_t monotonicEngineTicksCandidate(uint32_t currentWord,uint32_t&retainedWord){
 float current,retained;std::memcpy(&current,&currentWord,4);std::memcpy(&retained,&retainedWord,4);
 if(_mm_comigt_ss(_mm_set_ss(current),_mm_set_ss(retained)))retainedWord=currentWord;
 return engineSecondsTicksCandidate(retainedWord);
}
