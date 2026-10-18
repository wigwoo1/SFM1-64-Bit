#pragma once
#include "source_prepared_world_ray_candidate.h"
struct SourceSweptWorldTraceCandidate {
 SourcePreparedWorldRayStateCandidate state;
 std::array<float,3>startPosition{},endPosition{};
};
// Original 10145E20 for zero start-offset rays. Preserve its special fraction
// and left-solid branches and exact add/multiply order, including signed zero.
inline void sourceWorldTracePositionsCandidate(const std::array<float,3>&start,
 const std::array<float,3>&delta,SourceSweptWorldTraceCandidate&trace){
 auto staged=trace;auto&s=staged.state.trace;
 if(!std::isfinite(s.fraction)||s.fraction<0||s.fraction>1||!std::isfinite(s.leftSolid)||s.leftSolid<0||s.leftSolid>1)throw std::runtime_error("invalid finishing trace state");
 for(size_t a=0;a<3;++a){if(!std::isfinite(start[a])||!std::isfinite(delta[a]))throw std::runtime_error("invalid finishing ray");const float base=start[a]+0.f;
  if(s.fraction==1)staged.endPosition[a]=delta[a]+base;else{const float step=delta[a]*s.fraction;staged.endPosition[a]=step+base;}
  if(s.leftSolid==0)staged.startPosition[a]=base;else{if(s.leftSolid==1){s.allSolid=true;s.startSolid=true;s.fraction=0;staged.endPosition[a]=base;}const float step=delta[a]*s.leftSolid;staged.startPosition[a]=step+base;}
 }
 for(float v:staged.startPosition)if(!std::isfinite(v))throw std::runtime_error("finishing trace overflow");for(float v:staged.endPosition)if(!std::isfinite(v))throw std::runtime_error("finishing trace overflow");trace=staged;
}
inline SourceSweptWorldTraceCandidate sourceSweptWorldTraceCandidate(
 const SourcePreparedWorldRayCandidate&world,const std::array<float,3>&start,
 const std::array<float,3>&delta,uint32_t mask,bool computePositions=true,bool worldAvailable=true){
 SourceSweptWorldTraceCandidate result;if(!worldAvailable)return result;
 world.traceRay(start,delta,mask,result.state);if(computePositions)sourceWorldTracePositionsCandidate(start,delta,result);return result;
}
