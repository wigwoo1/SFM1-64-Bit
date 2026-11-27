#pragma once
#include "source_prepared_hull_world_tables_candidate.h"
#include "source_server_hull_ray_candidate.h"

struct SourceSweptHullWorldTraceCandidate {
 SourcePreparedWorldHullStateCandidate state;
 std::array<float,3> startPosition{},endPosition{};
 uint16_t worldSurfaceIndex=0;
};

// Original10145e20 coordinate finish for finite rays, including a nonzero
// start offset. Compute all end coordinates before the leftSolid==1 reset.
inline void sourceHullWorldTracePositionsCandidate(const SourceServerHullRayCandidate& ray,
 SourceSweptHullWorldTraceCandidate& trace){
 auto result=trace;auto& state=result.state.trace;
 if(!std::isfinite(state.fraction)||state.fraction<0||state.fraction>1||!std::isfinite(state.leftSolid)||state.leftSolid<0||state.leftSolid>1)throw std::runtime_error("invalid hull trace finishing state");
 std::array<float,3> base{};const float fraction=state.fraction,left=state.leftSolid;
 for(size_t a=0;a<3;++a){
  if(!std::isfinite(ray.center[a])||!std::isfinite(ray.offset[a])||!std::isfinite(ray.delta[a]))throw std::runtime_error("invalid finishing hull ray");
  base[a]=ray.offset[a]+ray.center[a];
  if(fraction==1.f)result.endPosition[a]=ray.delta[a]+base[a];
  else{const float step=ray.delta[a]*fraction;result.endPosition[a]=step+base[a];}
 }
 if(left==1.f){state.allSolid=true;state.startSolid=true;state.fraction=0;result.endPosition=base;}
 for(size_t a=0;a<3;++a){if(left==0.f)result.startPosition[a]=base[a];else{const float step=ray.delta[a]*left;result.startPosition[a]=step+base[a];}}
 for(float v:result.startPosition)if(!std::isfinite(v))throw std::runtime_error("hull trace start overflow");
 for(float v:result.endPosition)if(!std::isfinite(v))throw std::runtime_error("hull trace end overflow");
 trace=std::move(result);
}

// Swept non-ray branch of10149eb0 over explicit prepared world tables. Point
// and stationary dispatch are intentionally refused for an available world.
// SurfaceIdentity resolves brush TextureInfo/RuntimeSurface to uint16 native
// runtime namespace (UINT32_MAX denotes the default surface).
template<class SurfaceExcludes,class SurfaceDetails,class SurfaceIdentity>
SourceSweptHullWorldTraceCandidate sourceSweptHullWorldTraceCandidate(
 const SourceBspBrushOnlyTraceCandidate& world,
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const SourcePreparedHullWorldTablesCandidate& tables,
 const std::vector<std::array<uint16_t,2>>& properties,
 const SourceServerHullRayCandidate& ray,uint32_t mask,
 const std::array<uint16_t,2>& defaultSurface,bool computePositions,bool worldAvailable,
 SurfaceExcludes surfaceExcludes,SurfaceDetails surfaceDetails,SurfaceIdentity surfaceIdentity){
 SourceSweptHullWorldTraceCandidate result;result.state.surfaceProperty=defaultSurface[0];result.state.surfaceFlags=defaultSurface[1];
 if(!worldAvailable)return result;
 if(!ray.swept||ray.isRay)throw std::runtime_error("world hull wrapper requires swept non-ray input");
 sourcePreparedWorldHullCandidate(world,meshes,tables.caches,tables.leafDisplacements,tables.culls,properties,
  ray.center,ray.delta,ray.extents,mask,result.state,surfaceExcludes,surfaceDetails);
 if(result.state.worldSurfaceIndexSource){const auto id=surfaceIdentity(*result.state.worldSurfaceIndexSource);if(id!=UINT32_MAX&&id>=65535)throw std::runtime_error("world surface index outside16-bit namespace");result.worldSurfaceIndex=id==UINT32_MAX?65535:uint16_t(id);}
 if(computePositions)sourceHullWorldTracePositionsCandidate(ray,result);
 return result;
}
