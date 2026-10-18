#pragma once
#include "extended_product.h"
#include "extended_rounding.h"
#include <stdexcept>
struct NormalizedExtended{ExtendedLimbs significand{};uint16_t exponent=0;};
// Post-product path 0x10A1E3C9-0x10A1E4EC. Input exponent already adjusted.
inline NormalizedExtended normalizeExtendedProduct(ExtendedProduct product,int exponent){
 if(exponent< -32768||exponent>32767)throw std::runtime_error("adjusted exponent outside signed native domain");
 while(exponent>0&&!(product[5]&0x8000u)){
  unsigned carry=0;for(auto&w:product){unsigned next=w>>15;w=uint16_t((unsigned(w)<<1)|carry);carry=next;}--exponent;
 }
 if(exponent<=0){
  unsigned shifts=static_cast<unsigned>(1-exponent);bool sticky=false;
  for(unsigned n=0;n<shifts;++n){sticky|=(product[0]&1u)!=0;
   unsigned carry=0;for(int i=5;i>=0;--i){unsigned next=product[i]&1u;product[i]=uint16_t((product[i]>>1)|(carry<<15));carry=next;}
  }
  if(sticky)product[0]|=1;exponent=0;
 }
 if(extendedScaleRoundUp(uint32_t(product[0])|(uint32_t(product[1])<<16))){
  unsigned carry=1;for(unsigned i=1;i<6&&carry;++i){unsigned sum=unsigned(product[i])+carry;product[i]=uint16_t(sum);carry=sum>>16;}
  if(carry){product[5]=0x8000u;++exponent;}
 }
 NormalizedExtended result;
 if(exponent>=0x7fff){result.significand[4]=0x8000u;result.exponent=0x7fff;return result;}
 for(unsigned i=0;i<5;++i)result.significand[i]=product[i+1];
 result.exponent=uint16_t(exponent);return result;
}
