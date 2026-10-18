#pragma once
#include <array>
#include <cmath>
#include <xmmintrin.h>
//10066C51,10066800: SSE duration/elapsed then x87 exponential blend.
//Double libm replacement; exact x87 extended rounding remains unverified.
inline float evaluateRecoveredExponentialCurve(const std::array<float,4>&times,const std::array<float,4>&values,float fraction){
 auto span=_mm_sub_ss(_mm_set_ss(times[2]),_mm_set_ss(times[1]));float duration=_mm_cvtss_f32(span);
 if(!(duration>0))return values[1];
 float elapsed=_mm_cvtss_f32(_mm_mul_ss(span,_mm_set_ss(fraction)));
 const float base=0.001f;
 double exponent=(std::log(static_cast<double>(base))/duration)*elapsed;
 double weight=1.0-std::exp(exponent);
 return static_cast<float>(static_cast<double>(values[1])+(static_cast<double>(values[2])-values[1])*weight);
}
