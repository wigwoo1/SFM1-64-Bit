#pragma once
#include "extended_normalize.h"
inline NormalizedExtended multiplyExtended(NormalizedExtended left,NormalizedExtended right){
 unsigned sign=(left.exponent^right.exponent)&0x8000u;
 unsigned a=left.exponent&0x7fffu,b=right.exponent&0x7fffu,sum=a+b;
 auto infinity=[sign](){return NormalizedExtended{ExtendedLimbs{0,0,0,0,0x8000},uint16_t(0x7fffu|sign)};};
 if(a>=0x7fff||b>=0x7fff||sum>0xbffd)return infinity();
 if(sum<=0x3fbf)return {};
 auto zero=[](const ExtendedLimbs&v){for(auto w:v)if(w)return false;return true;};
 if(a==0){++sum;if(zero(left.significand))return {};}
 if(b==0){++sum;if(zero(right.significand))return {};}
 auto product=extendedPartialProduct(left.significand,right.significand);
 int exponent=int(sum)-0x3ffe;
 auto result=normalizeExtendedProduct(product,exponent);
 result.exponent=uint16_t(result.exponent|sign);return result;
}
