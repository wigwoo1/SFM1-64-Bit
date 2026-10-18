#pragma once
#include <array>
#include <cmath>
#include "native_gamma_index_candidate.h"
#include <stdexcept>
// SDK shader-constant policy, supported by the recovered installed RGB path.
// The caller must select the recovered x87 index rounding policy explicitly.
// Actual selfillum routing and authentic table values remain unverified here.
inline std::array<float,3> materialSdkTintGammaToLinearCandidate(
 const std::array<float,3>&authored,const std::array<float,256>&gammaTable,
 NativeGammaIndexRounding rounding){
 std::array<float,3>linear;
 for(size_t channel=0;channel<3;++channel){
  float value=authored[channel];if(!std::isfinite(value)||value<0)throw std::runtime_error("invalid authored material tint");
  if(value>1){linear[channel]=value;continue;}
  if(value>=.95f){linear[channel]=1;continue;}
  int index=nativeGammaIndexCandidate(value,rounding);
  if(index<0||index>=256)throw std::runtime_error("material tint gamma index outside table");
  linear[channel]=gammaTable[size_t(index)];
  if(!std::isfinite(linear[channel])||linear[channel]<0)throw std::runtime_error("invalid material tint gamma table value");
 }
 return linear;
}
