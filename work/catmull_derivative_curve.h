#pragma once
#include <xmmintrin.h>
//1049A7C0 derivative of raw Catmull component; no time normalization.
inline float evaluateRecoveredCatmullDerivative(float p0,float p1,float p2,float p3,float f){
 auto mul=[](float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));};auto add=[](float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 float quadratic=mul(mul(mul(f,3),f),.5f),linear=mul(mul(f,2),.5f);
 float value=add(add(add(mul(p0,-quadratic),mul(p1,mul(quadratic,3))),mul(p2,mul(quadratic,-3))),mul(p3,quadratic));
 value=add(add(add(add(value,mul(p0,mul(linear,2))),mul(p1,mul(linear,-5))),mul(p2,mul(linear,4))),mul(p3,-linear));
 return add(add(value,mul(p0,-.5f)),mul(p2,.5f));
}
