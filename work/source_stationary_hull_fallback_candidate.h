#pragma once
#include "source_prepared_world_hull_candidate.h"

struct SourceStationaryHullFallbackRayCandidate {
 std::array<float,3> start{},end{},delta{},inverse{},normal{};
};

// Geometry/state portion of1014ce00 for a preserved non-ray work flag.
// The host supplies nested visit/storage lifetime and actual hull tracing.
// Caller restores its previous end coordinates after using this temporary ray.
template<class TraceHull>
SourceStationaryHullFallbackRayCandidate sourceStationaryHullFallbackCandidate(
 const std::array<float,3>& point,const std::array<float,3>& normal,
 const std::array<uint16_t,2>& defaultSurface,SourcePreparedWorldHullStateCandidate& state,
 TraceHull traceHull){
 SourceStationaryHullFallbackRayCandidate ray;ray.start=point;ray.normal=normal;
 for(size_t a=0;a<3;++a){
  if(!std::isfinite(point[a])||!std::isfinite(normal[a]))throw std::runtime_error("invalid stationary hull fallback input");
  const float step=normal[a]*99999.8984375f;ray.end[a]=point[a]+step;ray.delta[a]=ray.end[a]-point[a];
  ray.inverse[a]=ray.delta[a]==0.f?std::numeric_limits<float>::max():1.f/ray.delta[a];
  if(!std::isfinite(ray.end[a])||!std::isfinite(ray.delta[a])||!std::isfinite(ray.inverse[a]))throw std::runtime_error("stationary hull fallback arithmetic overflow");
 }
 auto result=state;result.trace.fraction=1;result.trace.leftSolid=0;
 result.trace.allSolid=false;result.trace.startSolid=false;result.trace.surface={};
 result.displacementHit=0;result.surfaceProperty=defaultSurface[0];result.surfaceFlags=defaultSurface[1];
 // Contents, plane/tags, displacement flags and last-brush index survive reset.
 traceHull(ray,result);state=std::move(result);return ray;
}
