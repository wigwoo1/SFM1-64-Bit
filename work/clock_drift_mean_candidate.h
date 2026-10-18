#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <emmintrin.h>
extern "C" void clockDriftMeanIntervalX87(const uint32_t*,const uint32_t*,uint32_t*);
// engine10143820 plus caller10143cab multiplication, one final float store.
inline uint32_t clockDriftMeanIntervalCandidate(const std::array<uint32_t,16>&history,uint32_t intervalWord){
 auto sum=_mm_setzero_ps();
 for(auto word:history){float v;std::memcpy(&v,&word,4);sum=_mm_add_ss(sum,_mm_set_ss(v));}
 float v;_mm_store_ss(&v,sum);uint32_t sumWord,result;std::memcpy(&sumWord,&v,4);
 clockDriftMeanIntervalX87(&sumWord,&intervalWord,&result);return result;
}
