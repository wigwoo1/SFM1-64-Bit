#pragma once
#include <cmath>
#include <optional>
#include <cstdint>
#include <cstring>
// Native descriptor dispatch numeric results. Runtime error callbacks are
// separate; this helper does not reproduce their side effects.
inline std::optional<float> expressionModulo(float left,float right){
 uint32_t a,b;std::memcpy(&a,&left,4);std::memcpy(&b,&right,4);
 auto nan=[](uint32_t v){return (v&0x7fffffffu)>0x7f800000u;};
 uint32_t result;
 if(nan(a)||nan(b)){
  a=nan(a)?a|0x400000u:0;b=nan(b)?b|0x400000u:0;
  if((a&0x7fffffffu)==(b&0x7fffffffu))result=a&b;
  else result=(a&0x7fffffffu)>(b&0x7fffffffu)?a:b;
 }
 else if(!std::isfinite(left)||!std::isfinite(right)||right==0)result=0xffc00000u;
 else if(left==0)result=0;
 else return float(std::fmod(double(left),double(right)));
 std::memcpy(&left,&result,4);return left;
}
