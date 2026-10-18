#include "native_gamma_index_candidate.h"
#include <immintrin.h>
#include <iostream>
int main(){
 unsigned saved=_mm_getcsr(); unsigned checks=0;int result=0;
 for(unsigned ambient=0;ambient<4;++ambient){
  _mm_setcsr((saved&~0x6000u)|(ambient<<13));
  for(unsigned policy=0;policy<4;++policy){
   for(unsigned i=0;i<=4096;++i){
    float value=float(i)/8192.f;
    float scaled=value*255.f;
    unsigned before=_mm_getcsr();
    int actual=nativeGammaIndexCandidate(value,static_cast<NativeGammaIndexRounding>(policy));
    if((_mm_getcsr()&0x6000u)!=(before&0x6000u)){result=1;break;}
    _mm_setcsr((_mm_getcsr()&~0x6000u)|(policy<<13));
    int reference=_mm_cvtss_si32(_mm_set_ss(scaled));
    _mm_setcsr((_mm_getcsr()&~0x6000u)|(ambient<<13));
    if(actual!=reference){result=1;break;}++checks;
   }
  }
 }
 _mm_setcsr(saved);
 if(result)return result;
 std::cout<<"PASS x64 explicit gamma index: "<<checks<<" comparisons; ambient rounding control preserved\n";
}
