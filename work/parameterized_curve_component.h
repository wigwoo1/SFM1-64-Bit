#pragma once
#include "parameterized_curve_parameters.h"
#include <xmmintrin.h>
//1049B400 scalar component; callers apply1049B7E0 time normalization.
inline float evaluateRecoveredParameterizedComponent(float p0,float p1,float p2,float p3,float f,RecoveredParameterizedCurve p){
 auto add=[](float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));};auto sub=[](float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));};auto mul=[](float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 float scale=sub(1,p.first),plusC=mul(add(p.third,1),scale),minusC=mul(sub(1,p.third),scale),minusB=sub(1,p.second),plusB=add(p.second,1);
 float a=mul(plusB,plusC),b=mul(minusB,plusC),c=mul(minusC,plusB),d=mul(minusB,minusC);
 float square=mul(mul(f,f),.5f),cube=mul(square,f),half=mul(f,.5f);
 float value=add(add(add(mul(p1,mul(sub(sub(add(a,4),d),c),cube)),mul(p0,-mul(cube,a))),mul(p2,mul(sub(add(sub(d,4),c),b),cube))),mul(p3,mul(cube,b)));
 value=add(add(add(add(value,mul(p0,mul(mul(square,2),a))),mul(p1,mul(add(sub(mul(d,2),add(mul(a,2),6)),c),square))),mul(p2,mul(add(sub(sub(6,mul(d,2)),c),b),square))),mul(p3,-mul(square,b)));
 value=add(add(add(value,mul(p0,-mul(a,half))),mul(p1,mul(sub(a,d),half))),mul(p2,mul(d,half)));
 return add(value,p1);
}
