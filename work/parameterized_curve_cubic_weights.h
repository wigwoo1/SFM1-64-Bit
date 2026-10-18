#pragma once
#include <array>
#include <xmmintrin.h>
#include "parameterized_curve_parameters.h"
//1049B408..1049B579: cubic contributions; not a complete curve evaluator.
inline std::array<float,4> recoveredParameterizedCubicWeights(RecoveredParameterizedCurve p){
 auto add=[](float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));};auto sub=[](float a,float b){return _mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(a),_mm_set_ss(b)));};auto mul=[](float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 float scale=sub(1,p.first),plusC=mul(add(p.third,1),scale),minusC=mul(sub(1,p.third),scale),minusB=sub(1,p.second),plusB=add(p.second,1);
 float a=mul(plusB,plusC),b=mul(minusB,plusC),c=mul(minusC,plusB),d=mul(minusB,minusC);
 return {-a,sub(sub(add(a,4),d),c),sub(add(sub(d,4),c),b),b};
}
