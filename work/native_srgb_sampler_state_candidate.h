#pragma once
#include <cstdint>
#include <cstddef>
#include <stdexcept>
// Installed ShaderShadow slot20, SRGBRead diagnostic anchored at0x1002d280.
// Caller supplies allocated storage and the queried hardware sampler count.
// Bounds are stricter than original signed-upper-bound-only native behavior.
inline void setNativeSrgbSamplerStateCandidate(uint8_t*flags,size_t storage,
 int sampler,uint8_t enabled,bool capability,int hardwareCount){
 if(!flags||sampler<0||size_t(sampler)>=storage)
  throw std::runtime_error("sRGB sampler outside owned storage");
 if(capability&&(hardwareCount<0||sampler>=hardwareCount))
  throw std::runtime_error("sRGB sampler outside hardware count");
 auto&value=flags[size_t(sampler)];
 value=uint8_t((value&~2u)|(capability?((enabled&1u)<<1):0u));
}
