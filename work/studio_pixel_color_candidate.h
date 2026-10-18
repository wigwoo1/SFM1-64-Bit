#pragma once
#include "studio_pixel_constant_tail_candidate.h"
// The exposure callback supplies separately the stored float and the native
// extended-precision comparison result. Do not infer the latter from float32.
struct StudioPixelExposureCandidate{float stored;bool positive;};
template<class Ops>std::array<float,4> studioPixelColorCandidate(Ops& ops){
 const auto exposure=ops.exposure();
 std::array<float,4> local{0,0,0,exposure.positive?1.f/exposure.stored:0.f};
 const auto mode=ops.mode();
 if(mode==0)return local;
 if(mode==1){
  for(unsigned i=0;i<3;++i)local[i]=ops.channel(i);
  if(ops.transformCaptured())for(unsigned i=0;i<3;++i)local[i]=ops.transform(local[i]);
 }else{
  const bool power=ops.usePower();
  for(unsigned i=0;i<3;++i){
   const float input=ops.channel(i);
   local[i]=power?static_cast<float>(ops.power(static_cast<double>(input),2.200000047683716)):ops.transform(input);
  }
 }
 if(!ops.suppressScale()&&ops.serviceMode()==1){
  const float scale=ops.scale();
  for(unsigned i=0;i<3;++i)local[i]*=scale;
 }
 return local;
}
template<class Ops>bool studioPixelColorUploadCandidate(Ops& ops){
 const auto local=studioPixelColorCandidate(ops);
 std::array<uint32_t,4> bits{};
 std::memcpy(bits.data(),local.data(),16);
 return studioPixelConstantTailCandidate(bits,ops);
}
