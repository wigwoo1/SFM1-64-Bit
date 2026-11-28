#pragma once
#include "source_stationary_hull_world_trace_candidate.h"
#include "source_swept_point_world_trace_candidate.h"
#include "source_swept_hull_world_trace_candidate.h"

// Select the already prepared public route from the supplied Ray_t flags.
// All route-specific validation, default initialization and finishing remain
// in the existing helpers; this entry point only forwards the arguments.
template<class SurfaceExcludes,class SurfaceDetails,class SurfaceIdentity>
SourceSweptHullWorldTraceCandidate sourcePreparedWorldTraceCandidate(
 const SourceBspBrushOnlyTraceCandidate& world,
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const SourcePreparedHullWorldTablesCandidate& tables,
 const std::vector<std::array<uint16_t,2>>& properties,
 const SourceServerHullRayCandidate& ray,uint32_t mask,
 const std::array<uint16_t,2>& defaultSurface,bool computePositions,bool worldAvailable,
 SurfaceExcludes surfaceExcludes,SurfaceDetails surfaceDetails,SurfaceIdentity surfaceIdentity){
 if(!ray.swept)
  return sourceStationaryHullWorldTraceCandidate(world,meshes,tables,properties,ray,mask,defaultSurface,
   computePositions,worldAvailable,surfaceExcludes,surfaceDetails,surfaceIdentity);
 if(ray.isRay)
  return sourceSweptPointWorldTraceCandidate(world,meshes,tables,properties,ray,mask,defaultSurface,
   computePositions,worldAvailable,surfaceExcludes,surfaceDetails,surfaceIdentity);
 return sourceSweptHullWorldTraceCandidate(world,meshes,tables,properties,ray,mask,defaultSurface,
  computePositions,worldAvailable,surfaceExcludes,surfaceDetails,surfaceIdentity);
}
