#pragma once
#include <array>
#include <cstdint>
using ExtendedLimbs=std::array<uint16_t,5>;
using ExtendedProduct=std::array<uint16_t,6>;
// Native 0x10A1E359: truncated convolution, including local carry behavior.
inline ExtendedProduct extendedPartialProduct(const ExtendedLimbs&left,const ExtendedLimbs&right){
 ExtendedProduct result{};
 for(unsigned i=0;i<5;++i){
  for(unsigned k=0;k<5-i;++k){
   uint32_t old=uint32_t(result[i])|(uint32_t(result[i+1])<<16);
   uint32_t term=uint32_t(left[i+k])*right[4-k];
   uint32_t sum=old+term;
   result[i]=uint16_t(sum);result[i+1]=uint16_t(sum>>16);
   if(sum<old&&i+2<result.size())result[i+2]=uint16_t(result[i+2]+1);
  }
 }
 return result;
}
