#pragma once
#include <cstdint>
#include <cstring>
#include <emmintrin.h>
#include "material_sine_constants_candidate.h"
extern "C" double materialSineDoubleX87Candidate(double);

// Independently expressed SSE2 arithmetic and generated mathematical data.
// Requires round-to-nearest and nonflushing denormal SSE mode, as checked.
// reference. Floating status flags/alternate legacy CRT dispatch are unproved.
inline double materialSineDoubleCandidate(double value){
    uint64_t bits;std::memcpy(&bits,&value,8);const auto top=uint32_t((bits>>48)&0x7fffu);
    if(top<0x3030){if(top<16)return value*0x1.fffffffffffffp-1;return value;}
    if(top>0x40f5){if((bits&0x7ff0000000000000ull)==0x7ff0000000000000ull)return value*-0.0;return materialSineDoubleX87Candidate(value);}
    __m128d x0=_mm_set_sd(value),x1=_mm_mul_sd(_mm_set_sd(materialSinePiSplitCandidate[3]),x0);
    __m128d x2=_mm_set_sd(0x1.8p52);const auto index=uint32_t(_mm_cvtsd_si32(x1))&63u;
    x1=_mm_sub_sd(_mm_add_sd(x1,x2),x2);
    __m128d x3=_mm_set_sd(materialSinePiSplitCandidate[0]);
    x2=_mm_set1_pd(materialSinePiSplitCandidate[1]);x3=_mm_mul_sd(x3,x1);x1=_mm_unpacklo_pd(x1,x1);
    __m128d x4=x0,x5=_mm_set_pd(1.0/40320.0,1.0/362880.0);const auto*table=materialSineTableCandidate[index];
    x2=_mm_mul_pd(x2,x1);x0=_mm_sub_sd(x0,x3);x1=_mm_mul_sd(x1,_mm_set_sd(materialSinePiSplitCandidate[2]));x4=_mm_sub_sd(x4,x3);
    __m128d x7=_mm_load_sd(table+1);x0=_mm_unpacklo_pd(x0,x0);x3=_mm_move_sd(x3,x4);x4=_mm_sub_sd(x4,x2);
    x5=_mm_mul_pd(x5,x0);x0=_mm_sub_pd(x0,x2);
    __m128d x6=_mm_set_pd(1.0/24.0,1.0/120.0);x7=_mm_mul_sd(x7,x4);x3=_mm_sub_sd(x3,x4);x5=_mm_mul_pd(x5,x0);x0=_mm_mul_pd(x0,x0);x3=_mm_sub_sd(x3,x2);
    x2=_mm_loadu_pd(table);x1=_mm_sub_sd(x1,x3);x3=_mm_load_sd(table+3);x2=_mm_add_sd(x2,x3);x7=_mm_sub_sd(x7,x2);
    x2=_mm_mul_sd(x2,x4);x6=_mm_mul_pd(x6,x0);x3=_mm_mul_sd(x3,x4);x2=_mm_mul_pd(x2,x0);x0=_mm_mul_pd(x0,x0);
    x5=_mm_add_pd(x5,_mm_set_pd(-1.0/720.0,-1.0/5040.0));x4=_mm_mul_sd(x4,_mm_load_sd(table));x6=_mm_add_pd(x6,_mm_set_pd(-.5,-1.0/6.0));
    x5=_mm_mul_pd(x5,x0);x0=_mm_move_sd(x0,x3);x3=_mm_add_sd(x3,_mm_load_sd(table+1));x1=_mm_mul_pd(x1,x7);x7=_mm_move_sd(x7,x4);x4=_mm_add_sd(x4,x3);
    x6=_mm_add_pd(x6,x5);x5=_mm_load_sd(table+1);x5=_mm_sub_sd(x5,x3);x3=_mm_sub_sd(x3,x4);x1=_mm_add_sd(x1,_mm_load_sd(table+2));x6=_mm_mul_pd(x6,x2);
    x5=_mm_add_sd(x5,x0);x3=_mm_add_sd(x3,x7);x1=_mm_add_sd(x1,x5);x1=_mm_add_sd(x1,x3);x1=_mm_add_sd(x1,x6);x6=_mm_unpackhi_pd(x6,x6);x1=_mm_add_sd(x1,x6);x4=_mm_add_sd(x4,x1);
    return _mm_cvtsd_f64(x4);
}
