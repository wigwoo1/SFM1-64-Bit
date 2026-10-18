#pragma once
#include <cstdint>
#include <immintrin.h>
//100096f0 under masked FP exceptions. Explicit scalar SSE rounding.
inline bool studioShaderCloakOpaqueCandidate(float factor){
 float clamped=factor;if(factor>1.f)clamped=1.f;else if(0.f>factor)clamped=0.f;
 auto product=_mm_mul_ss(_mm_set_ss(clamped),_mm_set_ss(1.350000023841858f));
 float remaining=_mm_cvtss_f32(_mm_sub_ss(_mm_set_ss(1.f),product));
 if(remaining>1.f)return false;
 if(remaining<0.f)return true;
 return remaining<=0.4000000059604645f;
}
template<class Ops>bool studioShaderCloakGateCandidate(int32_t& factorIndex,Ops& ops){
 float factor=0.f;
 if(factorIndex>=0&&uint8_t(ops.defined(factorIndex)))factor=ops.value(factorIndex);
 return studioShaderCloakOpaqueCandidate(factor);
}
