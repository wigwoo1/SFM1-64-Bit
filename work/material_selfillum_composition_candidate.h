#pragma once
#include <array>
#include <cmath>
#include <stdexcept>
// SDK generic-shader equation reference; inputs are already in linear space.
// Explicit mask uses RGB; otherwise base alpha supplies all three channels.
// Tint conversion, lighting, fog and HDR output are separate renderer stages.
inline std::array<float,3> composeMaterialSelfIllumCandidate(
 const std::array<float,3>&diffuse,const std::array<float,3>&albedo,
 const std::array<float,3>&linearTint,float baseAlpha,
 const std::array<float,3>*explicitMask,bool enabled){
 for(float value:diffuse)if(!std::isfinite(value))throw std::runtime_error("nonfinite selfillum diffuse");
 if(!enabled)return diffuse;
 std::array<float,3>result{};
 for(size_t channel=0;channel<3;++channel){
  auto mask=explicitMask?(*explicitMask)[channel]:baseAlpha;
  if(!std::isfinite(mask)||mask<0||mask>1||!std::isfinite(albedo[channel])||!std::isfinite(linearTint[channel]))throw std::runtime_error("invalid selfillum composition input");
  float emission=linearTint[channel]*albedo[channel];
  result[channel]=diffuse[channel]+mask*(emission-diffuse[channel]);
  if(!std::isfinite(result[channel]))throw std::runtime_error("selfillum composition overflow");
 }
 return result;
}
