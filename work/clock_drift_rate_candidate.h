#pragma once
#include <cstdint>
#include <cstring>
#include <emmintrin.h>
inline uint32_t clockDriftRateCandidate(uint32_t inputWord,uint32_t minWord,uint32_t maxWord,uint32_t amountWord){
 float input,min,max,amount;std::memcpy(&input,&inputWord,4);std::memcpy(&min,&minWord,4);std::memcpy(&max,&maxWord,4);std::memcpy(&amount,&amountWord,4);
 auto value=_mm_set_ss(input),low=_mm_set_ss(min),high=_mm_set_ss(max);
 if(_mm_comigt_ss(value,high))value=high;
 else if(_mm_comigt_ss(low,value))value=low;
 auto rate=_mm_mul_ss(_mm_set_ss(amount),_mm_set_ss(.001f));
 // LAHF/TEST44/JP takes interpolation for unequal or unordered endpoints.
 if(_mm_movemask_ps(_mm_cmpeq_ss(low,high))&1){
  value=_mm_sub_ss(value,high);
  value=(_mm_movemask_ps(_mm_cmpnge_ss(value,_mm_setzero_ps()))&1)?_mm_setzero_ps():rate;
 }else{
  value=_mm_div_ss(_mm_mul_ss(_mm_sub_ss(value,low),rate),_mm_sub_ss(high,low));
 }
 float result;_mm_store_ss(&result,value);uint32_t word;std::memcpy(&word,&result,4);return word;
}
