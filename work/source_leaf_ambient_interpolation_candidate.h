#pragma once
#include "source_bsp_leaf_ambient_candidate.h"

// Reconstructed from checked engine.dll 100E15F0 and 102485C0. Not wired to
// rendering: native model anchor and HDR/LDR selection are separate contracts.
// Build with /fp:strict; scalar operation order is intentional.
struct SourceLeafAmbientInterpolationCandidate {
 using Cube=std::array<std::array<float,3>,6>;
 static float channel(uint8_t value,uint8_t encodedExponent){
  const int exponent=encodedExponent<128?encodedExponent:int(encodedExponent)-256;
  const float table=float(std::ldexp(1.0,exponent)/255.0);
  const float scaled=float(value)*table;
  return scaled*255.0f;
 }
 static Cube evaluate(const SourceBspLeafAmbientCandidate&data,uint32_t leaf,const std::array<float,3>&point){
  for(float value:point)if(!std::isfinite(value))throw std::runtime_error("nonfinite ambient query");
  const auto&initial=data.leaves.at(leaf);
  // Native follows exactly one nonzero reference. Do not use sampleOwner,
  // which intentionally supports a broader defensive data-inspection policy.
  if(initial.count==0&&initial.first!=0)leaf=initial.first;
  const auto&entry=data.leaves.at(leaf);Cube result{};
  if(entry.count==0)return result;
  if(size_t(entry.first)>data.samples.size()||size_t(entry.count)>data.samples.size()-entry.first)
   throw std::runtime_error("ambient interpolation span outside samples");
  float total=0.0f;
  for(size_t i=0;i<entry.count;++i){
   const auto&sample=data.samples[size_t(entry.first)+i];std::array<float,3>delta{};
   for(size_t axis=0;axis<3;++axis){
    const float center=(float(entry.mins[axis])+float(entry.maxs[axis]))*0.5f;
    const float half=float(entry.maxs[axis])-center;
    const float minimum=center-half;
    const float quantized=float(sample.position[axis])*half;
    const float offset=quantized*(2.0f/255.0f);
    const float position=offset+minimum;
    delta[axis]=position-point[axis];
   }
   const float xx=delta[0]*delta[0],yy=delta[1]*delta[1],zz=delta[2]*delta[2];
   const float xy=xx+yy;const float distance=xy+zz;const float denominator=distance+1.0f;
   if(!std::isfinite(denominator))throw std::runtime_error("ambient distance overflow");
   const float weight=1.0f/denominator;
   total=weight+total;
   for(size_t face=0;face<6;++face)for(size_t c=0;c<3;++c){
    const float value=channel(sample.cube[face][c],sample.cube[face][3]);
    const float weighted=value*weight;result[face][c]=result[face][c]+weighted;
   }
  }
  if(!(total>0.0f)||!std::isfinite(total))throw std::runtime_error("invalid ambient weight sum");
  const float inverse=1.0f/total;
  for(auto&face:result)for(float&value:face){value=value*inverse;if(!std::isfinite(value))throw std::runtime_error("ambient color overflow");}
  return result;
 }
};
