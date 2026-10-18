#pragma once
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <emmintrin.h>
inline int32_t recoveredFrameTicksCandidate(float frame,uint32_t packedRate,uint32_t mode){
 uint32_t numerator=packedRate&65535,denominator=packedRate>>16;if(!numerator||!denominator||!std::isfinite(frame)||(mode!=0&&mode!=1&&mode!=3))throw std::runtime_error("unsupported frame conversion");
 auto value=_mm_div_sd(_mm_mul_sd(_mm_set_sd(double(denominator)),_mm_set_sd(double(frame))),_mm_set_sd(double(numerator)));
 if(mode==3)return _mm_cvttsd_si32(value);if(mode==0)value=_mm_add_sd(value,_mm_set_sd(.5));
 auto sign=_mm_and_pd(_mm_set_sd(-0.0),value);auto magnitude=_mm_xor_pd(value,sign);auto threshold=_mm_set_sd(4503599627370496.0);auto mask=_mm_cmplt_sd(magnitude,threshold);auto bias=_mm_or_pd(_mm_and_pd(threshold,mask),sign);auto rounded=_mm_sub_sd(_mm_add_sd(value,bias),bias);auto correction=_mm_and_pd(_mm_cmpnle_sd(_mm_sub_sd(rounded,value),sign),_mm_set_sd(1));return _mm_cvttsd_si32(_mm_sub_sd(rounded,correction));
}
