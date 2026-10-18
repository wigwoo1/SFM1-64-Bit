#pragma once
#include <cstdint>
#include <xmmintrin.h>
struct RecoveredCurveKinds {uint32_t incoming,outgoing;};
// 10066930: high byte is incoming, low byte is outgoing; upper bits ignored.
inline RecoveredCurveKinds decodeRecoveredCurveKinds(int32_t packed){auto u=static_cast<uint32_t>(packed);return {(u>>8)&255,u&255};}
// 104FCA50 envelope. Evaluator supplies y from native 100669D0 dispatch.
// Four-point gathering and individual interpolation modes remain separate.
template<class Evaluate>
float evaluateRecoveredCurveEnvelope(float fraction,float first,float next,int32_t firstKind,int32_t nextKind,float minimum,float maximum,Evaluate evaluate){
 auto left=decodeRecoveredCurveKinds(firstKind).outgoing;
 auto right=decodeRecoveredCurveKinds(nextKind).incoming;
 float result;
 if(left==15)result=first;
 else if(right==15)result=next;
 else if(left==right)result=evaluate(left,fraction);
 else {auto a=_mm_set_ss(evaluate(left,fraction));auto b=_mm_set_ss(evaluate(right,fraction));result=_mm_cvtss_f32(_mm_add_ss(a,_mm_mul_ss(_mm_sub_ss(b,a),_mm_set_ss(fraction))));}
 if(result>maximum)return maximum;
 if(minimum>result)return minimum;
 return result;
}
