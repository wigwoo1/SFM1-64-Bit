#pragma once
#include <array>
#include <xmmintrin.h>
// 10499870: staged SSE squared length, sqrt and reciprocal multiplication.
inline std::array<float,4> normalizeRecoveredQuaternion(std::array<float,4> q){
 auto mul=[](float a,float b){return _mm_cvtss_f32(_mm_mul_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 auto add=[](float a,float b){return _mm_cvtss_f32(_mm_add_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 float squared=add(add(add(mul(q[0],q[0]),mul(q[1],q[1])),mul(q[2],q[2])),mul(q[3],q[3]));
 if(squared==0)return q;
 auto length=_mm_sqrt_ss(_mm_set_ss(squared));float reciprocal=_mm_cvtss_f32(_mm_div_ss(_mm_set_ss(1),length));
 for(auto&component:q)component=mul(component,reciprocal);return q;
}
