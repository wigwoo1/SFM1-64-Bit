#pragma once
#include <array>
#include "catmull_curve_component.h"
//1049AA70: normalize outer points by time/value chord length then1049A500.
inline float evaluateRecoveredChordCatmull(const std::array<float,4>&t,const std::array<float,4>&v,float fraction){
 auto add=[](float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));};auto sub=[](float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));};auto mul=[](float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 auto root=[](float a){return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(a)));};auto reciprocal=[](float a){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(1),_mm_set_ss(a)));};
 float dt=sub(t[2],t[1]),dv=sub(v[2],v[1]);float length=root(add(add(mul(dv,dv),0),mul(dt,dt)));
 float leftDt=sub(t[0],t[1]),leftDv=sub(v[0],v[1]),rightDt=sub(t[3],t[2]),rightDv=sub(v[3],v[2]);
 constexpr float epsilon=1.1920928955078125e-07f;
 float leftScale=reciprocal(add(root(add(add(mul(leftDv,leftDv),mul(leftDt,leftDt)),0)),epsilon));
 float rightScale=reciprocal(add(root(add(add(mul(rightDv,rightDv),mul(rightDt,rightDt)),0)),epsilon));
 float left=add(mul(mul(leftDv,leftScale),length),v[1]),right=add(mul(mul(rightDv,rightScale),length),v[2]);
 return evaluateRecoveredCatmullComponent(left,v[1],v[2],right,fraction);
}
