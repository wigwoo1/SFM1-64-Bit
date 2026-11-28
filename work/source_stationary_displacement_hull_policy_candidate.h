#pragma once
#include "source_displacement_box_overlap_candidate.h"
#include "source_stationary_displacement_hull_normal_candidate.h"
#include "source_stationary_hull_fallback_candidate.h"

struct SourceStationaryDisplacementHullOutcomeCandidate {
 uint32_t directMesh=UINT32_MAX;
 std::optional<SourceStationaryHullFallbackRayCandidate> fallbackRay;
};

inline bool sourceStationaryDisplacementCullBoxCandidate(const SourceLeafDisplacementCullCandidate& cull,
 const std::array<float,3>& minimum,const std::array<float,3>& maximum){
 for(size_t a=0;a<3;++a)if(minimum[a]>cull.maxs[a]||cull.mins[a]>maximum[a])return false;
 return true;
}

// Prepared stationary work branch of1014d320. Original leaf callers require a
// nonempty list. Relative hull bounds are centered (-extents,+extents), as
// prepared by the checked Ray_t constructor. Hosts supply actual geometry and
// nested visit/cache storage; these callbacks are not replacement hit results.
template<class BoundsOverlap,class MeshOverlap,class TraceHull>
SourceStationaryDisplacementHullOutcomeCandidate sourceStationaryDisplacementHullPolicyCandidate(
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const std::vector<SourceLeafDisplacementCullCandidate>& culls,const std::vector<uint32_t>& ordered,
 std::vector<uint32_t>& visited,uint32_t generation,const std::array<float,3>& point,
 const std::array<float,3>& relativeMinimum,const std::array<float,3>& relativeMaximum,uint32_t mask,
 const std::array<uint16_t,2>& defaultSurface,SourcePreparedWorldHullStateCandidate& state,
 BoundsOverlap boundsOverlap,MeshOverlap meshOverlap,TraceHull traceHull,bool isRay=false){
 if(ordered.empty()||meshes.size()!=culls.size()||!generation)throw std::runtime_error("stationary hull policy input coverage");
 bool nonzero=false;std::array<float,3> minimum{},maximum{};
 for(size_t a=0;a<3;++a){
  if(!std::isfinite(point[a])||!std::isfinite(relativeMinimum[a])||!std::isfinite(relativeMaximum[a])||relativeMinimum[a]>relativeMaximum[a]||relativeMinimum[a]!=-relativeMaximum[a])throw std::runtime_error("stationary hull policy requires finite centered bounds");
  nonzero|=relativeMinimum[a]!=0.f||relativeMaximum[a]!=0.f;
  minimum[a]=point[a]+relativeMinimum[a];maximum[a]=relativeMaximum[a]+point[a];
  if(!std::isfinite(minimum[a])||!std::isfinite(maximum[a]))throw std::runtime_error("stationary hull absolute bounds overflow");
 }
 for(auto id:ordered){const auto& c=culls.at(id);if(c.visitIndex>=visited.size())throw std::runtime_error("stationary hull visit reference");for(size_t a=0;a<3;++a)if(!std::isfinite(c.mins[a])||!std::isfinite(c.maxs[a])||c.mins[a]>c.maxs[a])throw std::runtime_error("invalid stationary hull cull bounds");}
 auto result=state;auto visits=visited;SourceStationaryDisplacementHullOutcomeCandidate outcome;
 if(nonzero)for(auto id:ordered){
  const auto& c=culls.at(id);if(!(c.contents&mask))continue;
  auto& mark=visits[c.visitIndex];if(mark==generation)continue;mark=generation;
  if(!boundsOverlap(id,minimum,maximum))continue;
  if(meshOverlap(id,minimum,maximum)){
   result.trace.allSolid=true;result.trace.startSolid=true;result.trace.fraction=0;result.trace.leftSolid=0;
   result.trace.contents=meshes.at(id).contents;outcome.directMesh=id;
   state=std::move(result);visited=std::move(visits);return outcome;
  }
 }
 std::array<float,3> normal{};uint32_t contents=0;
 sourceStationaryDisplacementHullNormalCandidate(meshes,ordered,point,relativeMinimum,relativeMaximum,isRay,mask,normal,contents);
 // Native passes contents to1014ce00, which does not consume that argument.
 outcome.fallbackRay=sourceStationaryHullFallbackCandidate(point,normal,defaultSurface,result,traceHull);
 if(result.displacementHit&&result.trace.startSolid){result.trace.allSolid=true;result.trace.fraction=0;result.trace.leftSolid=0;}
 else{result.trace.fraction=1;result.trace.allSolid=false;result.trace.startSolid=false;result.trace.contents=0;result.trace.leftSolid=0;}
 state=std::move(result);visited=std::move(visits);return outcome;
}

template<class TraceHull>
SourceStationaryDisplacementHullOutcomeCandidate sourceStationaryDisplacementHullCandidate(
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const std::vector<SourceLeafDisplacementCullCandidate>& culls,const std::vector<uint32_t>& ordered,
 std::vector<uint32_t>& visited,uint32_t generation,const std::array<float,3>& point,
 const std::array<float,3>& minimum,const std::array<float,3>& maximum,uint32_t mask,
 const std::array<uint16_t,2>& defaultSurface,SourcePreparedWorldHullStateCandidate& state,TraceHull traceHull,bool isRay=false){
 return sourceStationaryDisplacementHullPolicyCandidate(meshes,culls,ordered,visited,generation,point,minimum,maximum,mask,defaultSurface,state,
  [&](uint32_t id,const auto& low,const auto& high){return sourceStationaryDisplacementCullBoxCandidate(culls.at(id),low,high);},
  [&](uint32_t id,const auto& low,const auto& high){return sourceDisplacementBoxOverlapCandidate(meshes.at(id),low,high);},traceHull,isRay);
}
