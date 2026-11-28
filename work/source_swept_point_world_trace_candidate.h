#pragma once
#include "source_swept_hull_world_trace_candidate.h"
#include "source_prepared_point_world_candidate.h"

// Swept point branch of10149eb0 over explicit prepared geometry/material
// tables. Reuse the checked public result and coordinate finisher. The point
// route requires no hull cache/permutation resource. Work pool ownership and
// original whole-world loading remain outside this prepared adapter.
template<class SurfaceExcludes,class SurfaceDetails,class SurfaceIdentity>
SourceSweptHullWorldTraceCandidate sourceSweptPointWorldTraceCandidate(
 const SourceBspBrushOnlyTraceCandidate& world,
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const SourcePreparedHullWorldTablesCandidate& tables,
 const std::vector<std::array<uint16_t,2>>& properties,
 const SourceServerHullRayCandidate& ray,uint32_t mask,
 const std::array<uint16_t,2>& defaultSurface,bool computePositions,bool worldAvailable,
 SurfaceExcludes surfaceExcludes,SurfaceDetails surfaceDetails,SurfaceIdentity surfaceIdentity){
 SourceSweptHullWorldTraceCandidate result;result.state.surfaceProperty=defaultSurface[0];result.state.surfaceFlags=defaultSurface[1];
 if(!worldAvailable)return result;
 if(!ray.swept||!ray.isRay)throw std::runtime_error("point world wrapper requires swept point input");
 std::vector<uint32_t> brushVisits(world.data.brushes.size()),displacementVisits(meshes.size());
 sourcePreparedPointWorldCandidate(world,meshes,tables.leafDisplacements,tables.culls,properties,world.root,
  ray.center,ray.delta,ray.extents,mask,brushVisits,displacementVisits,1,result.state,surfaceExcludes,surfaceDetails);
 if(result.state.worldSurfaceIndexSource){const auto id=surfaceIdentity(*result.state.worldSurfaceIndexSource);if(id!=UINT32_MAX&&id>=65535)throw std::runtime_error("world surface index outside16-bit namespace");result.worldSurfaceIndex=id==UINT32_MAX?65535:uint16_t(id);}
 if(computePositions)sourceHullWorldTracePositionsCandidate(ray,result);
 return result;
}
