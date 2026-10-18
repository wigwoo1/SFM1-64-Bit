#pragma once
#include "vector_curve_dispatch.h"
#include "normalized_catmull_curve.h"
#include "curve_evaluation_envelope.h"
// 104FCC10: code 1 uses independent time-normalized scalar components.
inline std::array<float,3> evaluateRecoveredVectorWrapper(uint32_t code,const std::array<int32_t,4>&ticks,const std::array<std::array<float,3>,4>&points,float fraction){
 if(code!=1)return evaluateRecoveredVectorDispatch(code,points,fraction);
 std::array<float,4> times{};for(unsigned i=0;i<4;++i)times[i]=_mm_cvtss_f32(_mm_mul_ss(_mm_cvtsi32_ss(_mm_setzero_ps(),ticks[i]),_mm_set_ss(.0001f)));
 std::array<float,3> result{};for(unsigned c=0;c<3;++c){std::array<float,4> values{};for(unsigned i=0;i<4;++i)values[i]=points[i][c];result[c]=evaluateRecoveredNormalizedCatmull(times,values,fraction);}return result;
}
// 104FCE60: holds bypass interpolation; mixed codes blend componentwise.
inline std::array<float,3> evaluateRecoveredVectorEnvelope(const std::array<int32_t,4>&ticks,const std::array<std::array<float,3>,4>&points,float fraction,int32_t firstKind,int32_t nextKind){
 auto left=decodeRecoveredCurveKinds(firstKind).outgoing,right=decodeRecoveredCurveKinds(nextKind).incoming;
 if(left==15)return points[1];if(right==15)return points[2];
 auto result=evaluateRecoveredVectorWrapper(left,ticks,points,fraction);if(left==right)return result;
 auto next=evaluateRecoveredVectorWrapper(right,ticks,points,fraction);for(unsigned c=0;c<3;++c)result[c]=_mm_cvtss_f32(_mm_add_ss(_mm_set_ss(result[c]),_mm_mul_ss(_mm_sub_ss(_mm_set_ss(next[c]),_mm_set_ss(result[c])),_mm_set_ss(fraction))));return result;
}
