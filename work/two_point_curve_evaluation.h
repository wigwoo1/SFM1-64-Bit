#pragma once
#include <cstdint>
#include <cmath>
#include <emmintrin.h>
#include <stdexcept>
#include <xmmintrin.h>
// Direct SSE branches10066B21 (code4) and10066B97 (code6).
// Other dispatch modes require their recovered four-point helpers.
inline float evaluateRecoveredTwoPointCurve(uint32_t code,float fraction,float first,float next){
 auto f=_mm_set_ss(fraction);__m128 weight;
 if(code==10){auto square=_mm_mul_ss(f,f),cube=_mm_mul_ss(square,f);auto a=_mm_set_ss(first),b=_mm_set_ss(next);
  auto result=_mm_add_ss(_mm_mul_ss(b,_mm_mul_ss(cube,_mm_set_ss(-2))),_mm_mul_ss(a,_mm_mul_ss(cube,_mm_set_ss(2))));
  result=_mm_add_ss(result,_mm_mul_ss(a,_mm_mul_ss(square,_mm_set_ss(-3))));
  result=_mm_add_ss(result,_mm_mul_ss(b,_mm_mul_ss(square,_mm_set_ss(3))));return _mm_cvtss_f32(_mm_add_ss(a,result));
 }
 if(code==2||code==3){
  auto angle=_mm_mul_sd(_mm_mul_sd(_mm_cvtss_sd(_mm_setzero_pd(),f),_mm_set_sd(3.141592653589793)),_mm_set_sd(.5));
  if(code==3)angle=_mm_add_sd(angle,_mm_set_sd(1.5707963267948966));
  auto sine=_mm_set_sd(std::sin(_mm_cvtsd_f64(angle)));
  if(code==3)sine=_mm_sub_sd(_mm_set_sd(1.0),sine);
  weight=_mm_cvtsd_ss(_mm_setzero_ps(),sine);
 }
 else if(code==4){auto square=_mm_mul_ss(f,f);auto cubic=_mm_mul_ss(_mm_mul_ss(square,_mm_set_ss(2.0f)),f);weight=_mm_sub_ss(_mm_mul_ss(square,_mm_set_ss(3.0f)),cubic);}
 else if(code==6)weight=f;
 else throw std::runtime_error("curve code requires separate native interpolation helper");
 auto a=_mm_set_ss(first);return _mm_cvtss_f32(_mm_add_ss(_mm_mul_ss(_mm_sub_ss(_mm_set_ss(next),a),weight),a));
}
