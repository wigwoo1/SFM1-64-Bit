#pragma once
#include "extended_double_normal.h"
inline ExtendedLimbs shiftDoubleSignificand(ExtendedLimbs value,unsigned count){
 // No sticky-bit accumulation in 0x10A31820-0x10A3188A.
 if(count>=80)return {};
 while(count--){for(unsigned i=0;i<5;++i)value[i]=uint16_t((value[i]>>1)|(i<4?unsigned(value[i+1]&1u)<<15:0));}
 return value;
}
inline PackedDouble packExtendedDouble(NormalizedExtended value){
 const uint64_t sign=uint64_t(value.exponent&0x8000u)<<48;
 if((value.exponent&0x7fffu)==0){
  bool nonzero=false;for(auto word:value.significand)nonzero|=word!=0;
  return {sign,nonzero?2u:0u};
 }
 if(auto normal=packExtendedDoubleNormal(value))return *normal;
 const int originalExponent=int(value.exponent&0x7fffu)-0x3fff;
 const auto first=roundExtendedTo53(value.significand);
 if(originalExponent+(first.exponentCarry?1:0)<-1076)return {sign,2};
 const auto shifted=shiftDoubleSignificand(value.significand,unsigned(-1023-originalExponent));
 const auto rounded=roundExtendedTo53(shifted);
 // Native final shift is 12 rather than the normal path's 11.
 return {sign|(rounded.bits>>1),2};
}
