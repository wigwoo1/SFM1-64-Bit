#pragma once
#include "source_bsp_brush_only_hull_trace_candidate.h"
#include "source_displacement_world_hull_candidate.h"
#include "source_leaf_hull_displacement_policy_candidate.h"
#include <optional>

struct SourcePreparedWorldHullStateCandidate {
 SourceOptimizedBoxLineStateCandidate trace;
 uint16_t displacementFlags=0,surfaceProperty=0,surfaceFlags=0;
 uint32_t displacementHit=0;
 // Native trace+4a is updated on brush entry and retained on terrain hits or
 // default-surface resets. Keep its semantic source for the boundary resolver.
 std::optional<SourceCollisionSurfaceCandidate> worldSurfaceIndexSource;
};

// Swept hull world geometry over explicit prepared tables. Original BSP load,
// cache lifetime, stationary dispatch, entity/static-prop contacts and public
// trace setup/finish are separate. SurfaceDetails returns {property,flags} for
// a semantic brush surface; no native name/address is stored by this adapter.
// sweptVisits follows native work+101. A nested stationary normal probe keeps
// it false while still executing this hull geometry kernel; this is not the
// stationary overlap dispatcher itself.
template<class SurfaceExcludes,class SurfaceDetails>
void sourcePreparedWorldHullCandidate(const SourceBspBrushOnlyTraceCandidate& world,
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const std::vector<SourceDisplacementHullCacheCandidate>& caches,
 const std::vector<std::vector<uint32_t>>& leafDisplacements,
 const std::vector<SourceLeafDisplacementCullCandidate>& culls,
 const std::vector<std::array<uint16_t,2>>& properties,
 const std::array<float,3>& center,const std::array<float,3>& delta,
 const std::array<float,3>& extents,uint32_t mask,SourcePreparedWorldHullStateCandidate& state,
 SurfaceExcludes surfaceExcludes,SurfaceDetails surfaceDetails,bool sweptVisits=true) {
 if(meshes.size()!=caches.size()||meshes.size()!=culls.size()||meshes.size()!=properties.size()||leafDisplacements.size()!=world.data.leafBrushes.size())throw std::runtime_error("prepared hull world table coverage");
 auto result=state;std::array<float,3> end{},inverse{};
 for(size_t a=0;a<3;++a){end[a]=center[a]+delta[a];inverse[a]=delta[a]==0?std::numeric_limits<float>::max():1.f/delta[a];if(!std::isfinite(delta[a])||!std::isfinite(inverse[a])||!std::isfinite(end[a]))throw std::runtime_error("invalid prepared hull movement");}
 std::vector<uint32_t> contents;for(const auto& b:world.data.brushes)contents.push_back(b.contents);
 std::vector<uint32_t> brushVisits(contents.size()),dispVisits(meshes.size());
 sourceBspHullWalkCandidate(world.planes,world.nodes,world.root,uint32_t(leafDisplacements.size()),center,end,extents,0,1,result.trace.fraction,
  [&](uint32_t leaf,float,float){
   const auto& brushes=world.data.leafBrushes.at(leaf);
   sourceLeafBrushPolicyCandidate(brushes,contents,brushVisits,1,mask,result.trace,surfaceExcludes,
    [&](uint32_t brush,auto& hit){
     const float beforeFraction=hit.fraction,beforeLeft=hit.leftSolid;bool entering=false;
     sourceBrushHullDispatchCandidate(world.data,world.records,brush,center,end,delta,extents,hit,&entering);
     // Both native brush kernels can reset the surface after a solid exit
     // extends beyond the current hit. That reset retains the displacement
     // marker but still copies the default material property/flags.
     const bool exitReset=!entering&&hit.surface.kind==SourceCollisionSurfaceKindCandidate::Default&&hit.fraction==1.f&&hit.leftSolid>beforeLeft&&hit.leftSolid>=beforeFraction;
     if(entering){result.displacementHit=0;result.worldSurfaceIndexSource=hit.surface;}
     if(entering||exitReset){const auto details=surfaceDetails(hit.surface);result.surfaceProperty=details[0];result.surfaceFlags=details[1];}
    });
   SourceLeafDisplacementStateCandidate phase;phase.fraction=result.trace.fraction;phase.startSolid=result.trace.startSolid;phase.allSolid=result.trace.allSolid;phase.displacementHit=result.displacementHit!=0;std::copy_n(result.trace.plane.begin(),3,phase.normal.begin());
   sourceLeafHullDisplacementPolicyCandidate(leafDisplacements.at(leaf),culls,dispVisits,1,mask,sweptVisits,!brushes.empty(),phase,center,delta,inverse,extents,
    [&](uint32_t index,auto& hit){
     SourceDisplacementWorldHullStateCandidate d;d.hull.fraction=hit.fraction;d.hull.normal=hit.normal;d.hull.distance=result.trace.plane[3];d.hull.displacementFlags=result.displacementFlags;d.contents=result.trace.contents;d.displacementHit=result.displacementHit;d.surfaceIsDisplacement=result.trace.surface.kind==SourceCollisionSurfaceKindCandidate::Displacement;d.surfaceProperty=result.surfaceProperty;d.surfaceFlags=result.surfaceFlags;
     const float before=hit.fraction;sourceDisplacementWorldHullCandidate(meshes.at(index),caches.at(index),properties.at(index),center,delta,inverse,extents,d);
     if(d.hull.fraction<before)result.trace.surface={SourceCollisionSurfaceKindCandidate::Displacement,index};
     hit.fraction=d.hull.fraction;hit.normal=d.hull.normal;hit.displacementHit=d.displacementHit!=0;
     result.trace.plane={d.hull.normal[0],d.hull.normal[1],d.hull.normal[2],d.hull.distance};result.trace.contents=d.contents;result.displacementFlags=d.hull.displacementFlags;result.displacementHit=d.displacementHit;result.surfaceProperty=d.surfaceProperty;result.surfaceFlags=d.surfaceFlags;
    });
   result.trace.fraction=phase.fraction;result.trace.startSolid=phase.startSolid;result.trace.allSolid=phase.allSolid;
  });state=result;
}
