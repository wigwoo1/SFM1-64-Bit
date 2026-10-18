#include "extended_double.h"
#include <iostream>
int main(){unsigned count=0;
 for(unsigned negative=0;negative<2;++negative){
  uint16_t sign=negative?0x8000:0;uint64_t expectedSign=uint64_t(negative)<<63;
  for(int exponent=-1100;exponent<=-1023;++exponent){
   NormalizedExtended value{{0,0,0,0,0x8000},uint16_t(exponent+0x3fff+sign)};
   auto result=packExtendedDouble(value);
   uint64_t expected=exponent>=-1074?uint64_t(1)<<(exponent+1074):0;
   if(result.bits!=(expected|expectedSign)||result.status!=2)return 1;++count;
  }
  auto zero=packExtendedDouble({{},sign});
  auto denormal=packExtendedDouble({{1,0,0,0,0},sign});
  if(zero.bits!=expectedSign||zero.status||denormal.bits!=expectedSign||denormal.status!=2)return 2;count+=2;
 }
 // The recovered two-stage rounding truncates 0.75 minimum-subnormal to zero.
 auto tiny=packExtendedDouble({{0,0,0,0,0xc000},0x3bcc});
 if(tiny.bits||tiny.status!=2)return 3;++count;
 // Largest extended value immediately below the minimum normal rounds across it.
 auto crossover=packExtendedDouble({{0xffff,0xffff,0xffff,0xffff,0xffff},0x3c00});
 if(crossover.bits!=0x0010000000000000ull||crossover.status!=0)return 4;++count;
 std::cout<<count<<" full packing underflow/zero boundary cases passed\n";
}
