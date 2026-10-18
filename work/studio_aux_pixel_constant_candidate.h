#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include "studio_pixel_constant_tail_candidate.h"
// Logical SSE float calculation for100144a0; native FP exceptions/rounding
// require separate verification. Inactive sentinel is exact installed bits.
template<class Ops>std::array<uint32_t,4> studioAuxPixelConstantCandidate(uint32_t index,uint32_t enabled,Ops& ops){
 ops.publishIndex(index);
 const auto mode=ops.mode();
 if(!mode||!enabled)return {0,0xff7fffff,0,0};
 const float lo=ops.lower(),hi=ops.upper();
 const float inverse=hi==lo?1.f:1.f/(hi-lo);
 const float first=1.f-hi*inverse;
 const float second=ops.second();
 float density=ops.density();
 if(density>1.f)density=1.f;
 else if(density<0.f)density=0.f;
 const std::array<float,4> local{first,second,density,inverse};
 std::array<uint32_t,4> bits{};std::memcpy(bits.data(),local.data(),16);return bits;
}
template<class Ops>bool studioAuxPixelConstantUploadCandidate(uint32_t index,uint32_t enabled,Ops& ops){
 const auto local=studioAuxPixelConstantCandidate(index,enabled,ops);
 return studioPixelConstantBlockCandidate(index,local,ops);
}
