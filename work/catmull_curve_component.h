#pragma once
#include <xmmintrin.h>
// Scalar component of1049A500 (curve code11). Preserve staged SSE sums.
inline float evaluateRecoveredCatmullComponent(float p0,float p1,float p2,float p3,float f){
 auto mul=[](float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 auto add=[](float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 float square=mul(mul(f,f),.5f),cube=mul(square,f),half=mul(f,.5f);
 float value=add(add(add(mul(p0,-cube),mul(p1,mul(cube,3))),mul(p2,mul(cube,-3))),mul(p3,cube));
 value=add(add(add(add(value,mul(p0,mul(square,2))),mul(p1,mul(square,-5))),mul(p2,mul(square,4))),mul(p3,-square));
 value=add(add(value,mul(p0,-half)),mul(p2,half));
 return add(value,p1);
}
