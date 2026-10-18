#pragma once
#include "time_selection.h"
inline float timeFraction(int32_t tick,int32_t start,int32_t end,bool clamp){
 auto n=wrapTicks(uint32_t(tick)-uint32_t(start)),d=wrapTicks(uint32_t(end)-uint32_t(start));if(!d)return 0;
 if(clamp){if(n>d)n=d;else if(n<0)n=0;}
 return _mm_cvtss_f32(_mm_div_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),n),_mm_cvtsi32_ss(_mm_setzero_ps(),d)));
}
inline float interpolateFloat(float first,float next,float fraction){
 auto f=_mm_set_ss(fraction);auto a=_mm_mul_ss(_mm_sub_ss(_mm_set_ss(1),f),_mm_set_ss(first));auto b=_mm_mul_ss(_mm_set_ss(next),f);return _mm_cvtss_f32(_mm_add_ss(a,b));
}
