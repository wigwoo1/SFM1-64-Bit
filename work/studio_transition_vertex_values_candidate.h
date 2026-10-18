#pragma once
#include "studio_shader_constant_pair_candidate.h"
// Value stage1001b980..1001bad2 only. Caller must resolve earlier skip/mode7
// branches; modern capability with false input flag skips this stage entirely.
template<class Ops>std::array<uint32_t,4> studioTransitionVertexValuesCandidate(bool active,Ops& ops){
 float hi=0,inverse=0;
 if(active){hi=ops.upper();const float lo=ops.lower();inverse=hi==lo?1.f:1.f/(hi-lo);}
 const bool modern=ops.capability()>90;
 if(!active)return modern?std::array<uint32_t,4>{0,0xff7fffff,0,0}:std::array<uint32_t,4>{0x3f800000,0xff7fffff,0x3f800000,0};
 const float product=hi*inverse;
 float density=ops.density();if(density>1)density=1;else if(density<0)density=0;
 const std::array<float,4> local{modern?1.f-product:product,0,modern?density:1.f-density,inverse};
 std::array<uint32_t,4> bits{};std::memcpy(bits.data(),local.data(),16);return bits;
}
