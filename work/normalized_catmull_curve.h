#pragma once
#include <array>
#include "catmull_curve_component.h"
// 1049AC20: adjust outer points to middle time span before1049A500.
inline std::array<float,4> normalizeRecoveredCurveValues(const std::array<float,4>&times,const std::array<float,4>&values){
 auto sub=[](float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 auto div=[](float a,float b){return _mm_cvtss_f32(_mm_div_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 auto adjust=[&](float outer,float middle,float factor){return _mm_cvtss_f32(_mm_add_ss(_mm_mul_ss(_mm_set_ss(sub(outer,middle)),_mm_set_ss(factor)),_mm_set_ss(middle)));};
 float left=values[0],right=values[3],span=sub(times[2],times[1]);
 // Native parity test skips ordered equality only; unordered compares proceed.
 if(span!=0){
  if(times[0]!=times[1])left=adjust(left,values[1],div(span,sub(times[1],times[0])));
  if(times[3]!=times[2])right=adjust(right,values[2],div(span,sub(times[3],times[2])));
 }
 return {left,values[1],values[2],right};
}

inline float evaluateRecoveredNormalizedCatmull(const std::array<float,4>&times,const std::array<float,4>&values,float fraction){auto v=normalizeRecoveredCurveValues(times,values);return evaluateRecoveredCatmullComponent(v[0],v[1],v[2],v[3],fraction);}
