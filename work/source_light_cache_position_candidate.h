#pragma once
#include "source_light_cache_cell_candidate.h"
struct SourceLightCacheTraceCandidate {float fraction=0.0f;bool startSolid=false;};
// Decision policy recovered from engine 101947B0. World must provide real leaf,
// contents, constrainToStartLeaf and trace queries before renderer use.
// Tests may supply scripted queries; those do not establish collision parity.
template<class World>
std::array<float,3> sourceLightCachePositionCandidate(const std::array<float,3>&point,int32_t inputLeaf,World&world){
 const auto bounds=sourceLightCacheCellCandidate(point);
 std::array<float,3>candidate{};
 for(size_t axis=0;axis<3;++axis)candidate[axis]=(bounds[axis]+bounds[axis+3])*0.5f;
 const auto fullCenter=candidate;
 const int32_t centerLeaf=world.leafAt(candidate);
 bool horizontalFallback=false;
 if(centerLeaf!=inputLeaf){
  if((world.contents(centerLeaf)&0x4081u)!=0)horizontalFallback=true;
  else world.constrainToStartLeaf(point,0.5f,candidate);
 }
 auto checkedTrace=[&](){
  for(float value:candidate)if(!std::isfinite(value))throw std::runtime_error("nonfinite constrained cache position");
  const auto trace=world.trace(point,candidate,0x4081u);
  if(!std::isfinite(trace.fraction)||trace.fraction<0.0f||trace.fraction>1.0f)throw std::runtime_error("invalid cache trace fraction");
  return trace;
 };
 if(!horizontalFallback){
  const auto trace=checkedTrace();
  if(trace.startSolid)return point;
  if(trace.fraction>=1.0f)return candidate;
 }
 candidate={fullCenter[0],fullCenter[1],point[2]};
 world.constrainToStartLeaf(point,0.5f,candidate);
 const auto trace=checkedTrace();
 // The original second-trace branch checks fraction only, not startSolid.
 return trace.fraction>=1.0f?candidate:point;
}
