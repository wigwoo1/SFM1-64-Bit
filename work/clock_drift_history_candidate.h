#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <emmintrin.h>
// Recovered engine10143870. Explicit state, default masked FP environment.
inline void clockDriftHistoryCandidate(std::array<uint32_t,16>& words,uint32_t correctionWord,uint32_t intervalWord){
 std::array<float,16>history;std::memcpy(history.data(),words.data(),sizeof(history));
 __m128 sum=_mm_setzero_ps();
 for(unsigned block=0;block<2;++block){
  sum=_mm_add_ss(_mm_set_ss(history[block*8]),sum);
  for(unsigned j=1;j<8;++j)sum=_mm_add_ss(sum,_mm_set_ss(history[block*8+j]));
 }
 auto mean=_mm_mul_ss(sum,_mm_set_ss(.0625f));
 if(_mm_comigt_ss(_mm_set_ss(.05f),mean))return;
 float correction,interval;std::memcpy(&correction,&correctionWord,4);std::memcpy(&interval,&intervalWord,4);
 auto factor=_mm_add_ss(_mm_div_ss(_mm_div_ss(_mm_set_ss(correction),_mm_set_ss(interval)),mean),_mm_set_ss(1));
 for(unsigned i=0;i<16;++i){auto v=_mm_set_ss(history[i]);v=i%8==5?_mm_mul_ss(factor,v):_mm_mul_ss(v,factor);_mm_store_ss(&history[i],v);}
 std::memcpy(words.data(),history.data(),sizeof(history));
}
