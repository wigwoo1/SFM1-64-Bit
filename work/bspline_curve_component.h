#pragma once
#include <xmmintrin.h>
//1049BA80 component, dispatch code5. Keep native staged SSE accumulation.
inline float evaluateRecoveredBSplineComponent(float p0,float p1,float p2,float p3,float f){
 auto mul=[](float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));};auto add=[](float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 constexpr float sixth=0.1666666716337204f;
 float square=mul(mul(f,f),sixth),cube=mul(square,f),linear=mul(f,sixth);
 float value=add(add(add(mul(p0,-cube),mul(p1,mul(cube,3))),mul(p2,mul(cube,-3))),mul(p3,cube));
 value=add(add(add(value,mul(p0,mul(square,3))),mul(p1,mul(square,-6))),mul(p2,mul(square,3)));
 value=add(add(value,mul(p0,mul(linear,-3))),mul(p2,mul(linear,3)));
 value=add(add(add(value,mul(p0,sixth)),mul(p1,0.6666666865348816f)),mul(p2,sixth));return value;
}
