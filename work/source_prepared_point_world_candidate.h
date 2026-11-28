#pragma once
#include "source_prepared_world_hull_candidate.h"
#include "source_bsp_point_walk_candidate.h"
#include "source_displacement_world_ray_candidate.h"
#include "source_leaf_displacement_policy_candidate.h"

struct SourcePreparedPointWorldEventsCandidate {
 std::vector<uint32_t> leaves,brushes,displacements;
 std::vector<std::array<float,2>> intervals;
};

// Complete prepared point geometry with common full trace/material state.
// Original point route10148950 and leaf10148270; public setup/finish and global
// storage ownership are outside this function. Preserve supplied delta and the
// work swept flag: stationary normal probes still use this geometry route.
template<class SurfaceExcludes,class SurfaceDetails>
void sourcePreparedPointWorldCandidate(const SourceBspBrushOnlyTraceCandidate& world,
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const std::vector<std::vector<uint32_t>>& leafDisplacements,
 const std::vector<SourceLeafDisplacementCullCandidate>& culls,
 const std::vector<std::array<uint16_t,2>>& properties,int32_t root,
 const std::array<float,3>& start,const std::array<float,3>& delta,const std::array<float,3>& axialExtents,
 uint32_t mask,std::vector<uint32_t>& brushVisits,std::vector<uint32_t>& displacementVisits,uint32_t generation,
 SourcePreparedWorldHullStateCandidate& state,SurfaceExcludes surfaceExcludes,SurfaceDetails surfaceDetails,
 bool sweptVisits=true,SourcePreparedPointWorldEventsCandidate* events=nullptr){
 if(!generation||meshes.size()!=culls.size()||meshes.size()!=properties.size()||displacementVisits.size()!=meshes.size()||brushVisits.size()!=world.data.brushes.size()||leafDisplacements.size()!=world.data.leafBrushes.size())throw std::runtime_error("prepared point world table coverage");
 auto result=state;auto brushMarks=brushVisits,dispMarks=displacementVisits;SourcePreparedPointWorldEventsCandidate observed;
 std::array<float,3> end{},inverse{};
 for(size_t a=0;a<3;++a){end[a]=start[a]+delta[a];inverse[a]=delta[a]==0.f?std::numeric_limits<float>::max():1.f/delta[a];if(!std::isfinite(delta[a])||!std::isfinite(inverse[a])||!std::isfinite(end[a]))throw std::runtime_error("invalid prepared point movement");}
 std::vector<uint32_t> contents;for(const auto& brush:world.data.brushes)contents.push_back(brush.contents);
 sourceBspPointWalkCandidate(world.planes,world.nodes,root,uint32_t(leafDisplacements.size()),start,end,axialExtents,0,1,result.trace.fraction,
  [&](uint32_t leaf,float from,float to){
   if(events){observed.leaves.push_back(leaf);observed.intervals.push_back({from,to});}const auto& brushes=world.data.leafBrushes.at(leaf);
   sourceLeafBrushPolicyCandidate(brushes,contents,brushMarks,generation,mask,result.trace,surfaceExcludes,
    [&](uint32_t brush,auto& hit){
     if(events)observed.brushes.push_back(brush);
     const float beforeFraction=hit.fraction,beforeLeft=hit.leftSolid;bool entering=false;
     sourceBrushLineDispatchCandidate(world.data,world.records,brush,start,end,hit,&entering,&delta);
     const bool exitReset=!entering&&hit.surface.kind==SourceCollisionSurfaceKindCandidate::Default&&hit.fraction==1.f&&hit.leftSolid>beforeLeft&&hit.leftSolid>=beforeFraction;
     if(entering){result.displacementHit=0;result.worldSurfaceIndexSource=hit.surface;}
     if(entering||exitReset){const auto details=surfaceDetails(hit.surface);result.surfaceProperty=details[0];result.surfaceFlags=details[1];}
    });
   SourceLeafDisplacementStateCandidate phase;phase.fraction=result.trace.fraction;phase.startSolid=result.trace.startSolid;phase.allSolid=result.trace.allSolid;phase.displacementHit=result.displacementHit!=0;std::copy_n(result.trace.plane.begin(),3,phase.normal.begin());
   sourceLeafDisplacementPolicyCandidate(leafDisplacements.at(leaf),culls,dispMarks,generation,mask,sweptVisits,!brushes.empty(),phase,start,delta,inverse,
    [&](uint32_t index,auto& hit){
     if(events)observed.displacements.push_back(index);
     SourceDisplacementWorldRayStateCandidate d;d.ray.fraction=hit.fraction;d.ray.normal=hit.normal;d.ray.distance=result.trace.plane[3];d.ray.displacementFlags=result.displacementFlags;d.contents=result.trace.contents;d.displacementHit=result.displacementHit;d.surfaceIsDisplacement=result.trace.surface.kind==SourceCollisionSurfaceKindCandidate::Displacement;d.surfaceProperty=result.surfaceProperty;d.surfaceFlags=result.surfaceFlags;
     const float before=hit.fraction;sourceDisplacementWorldRayCandidate(meshes.at(index),properties.at(index),start,delta,inverse,d);
     if(d.ray.fraction<before)result.trace.surface={SourceCollisionSurfaceKindCandidate::Displacement,index};
     hit.fraction=d.ray.fraction;hit.normal=d.ray.normal;hit.displacementHit=d.displacementHit!=0;
     result.trace.plane={d.ray.normal[0],d.ray.normal[1],d.ray.normal[2],d.ray.distance};result.trace.contents=d.contents;result.displacementFlags=d.ray.displacementFlags;result.displacementHit=d.displacementHit;result.surfaceProperty=d.surfaceProperty;result.surfaceFlags=d.surfaceFlags;
    });
   result.trace.fraction=phase.fraction;result.trace.startSolid=phase.startSolid;result.trace.allSolid=phase.allSolid;
  });
 state=std::move(result);brushVisits=std::move(brushMarks);displacementVisits=std::move(dispMarks);if(events)*events=std::move(observed);
}
