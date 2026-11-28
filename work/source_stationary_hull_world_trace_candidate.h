#pragma once
#include "source_stationary_hull_world_candidate.h"
#include "source_swept_hull_world_trace_candidate.h"

// Nonswept non-ray public branch10149eb0. Reuse the existing trace result and
// coordinate finisher without changing the separately verified swept entry.
// Preserve the supplied delta: squared-length underflow can classify a finite
// nonzero movement as stationary. Empty worlds return before ray/geometry use.
template<class SurfaceExcludes,class SurfaceDetails,class SurfaceIdentity>
SourceSweptHullWorldTraceCandidate sourceStationaryHullWorldTraceCandidate(
 const SourceBspBrushOnlyTraceCandidate& world,
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const SourcePreparedHullWorldTablesCandidate& tables,
 const std::vector<std::array<uint16_t,2>>& properties,
 const SourceServerHullRayCandidate& ray,uint32_t mask,
 const std::array<uint16_t,2>& defaultSurface,bool computePositions,bool worldAvailable,
 SurfaceExcludes surfaceExcludes,SurfaceDetails surfaceDetails,SurfaceIdentity surfaceIdentity){
 SourceSweptHullWorldTraceCandidate result;result.state.surfaceProperty=defaultSurface[0];result.state.surfaceFlags=defaultSurface[1];
 if(!worldAvailable)return result;
 if(ray.swept||ray.isRay)throw std::runtime_error("stationary hull wrapper requires nonswept non-ray input");
 std::vector<uint32_t> brushVisits(world.data.brushes.size()),displacementVisits(meshes.size());
 sourceStationaryHullWorldCandidate(world,meshes,tables.leafDisplacements,tables.culls,world.root,ray.center,ray.extents,
  brushVisits,displacementVisits,1,mask,defaultSurface,result.state,
  [&](const auto& probe,auto& output){sourcePreparedWorldHullCandidate(world,meshes,tables.caches,tables.leafDisplacements,tables.culls,properties,
   probe.start,probe.delta,ray.extents,mask,output,surfaceExcludes,surfaceDetails,false);});
 if(result.state.worldSurfaceIndexSource){const auto id=surfaceIdentity(*result.state.worldSurfaceIndexSource);if(id!=UINT32_MAX&&id>=65535)throw std::runtime_error("world surface index outside16-bit namespace");result.worldSurfaceIndex=id==UINT32_MAX?65535:uint16_t(id);}
 if(computePositions)sourceHullWorldTracePositionsCandidate(ray,result);
 return result;
}
