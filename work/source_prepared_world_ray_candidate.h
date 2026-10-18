#pragma once
#include "source_displacement_leaf_assignment_candidate.h"
#include "source_displacement_world_ray_candidate.h"
#include "source_leaf_displacement_policy_candidate.h"
#include "source_stationary_displacement_normal_candidate.h"
#include "source_stationary_leaf_selection_candidate.h"
#include "source_stationary_brush_candidate.h"

struct SourcePreparedWorldRayStateCandidate {
 SourceOptimizedBoxLineStateCandidate trace;
 uint16_t displacementFlags=0;
 uint32_t displacementHit=0;
};

// Zero-extent completion branch 1014D4AA..1014D534. A failed solid test
// clears contents but deliberately retains the probe's plane, flags and marker.
inline void sourceStationaryDisplacementFinishCandidate(SourcePreparedWorldRayStateCandidate&result){
 if(result.displacementHit&&result.trace.startSolid){result.trace.allSolid=true;result.trace.fraction=0;result.trace.leftSolid=0;}
 else{result.trace.fraction=1;result.trace.leftSolid=0;result.trace.allSolid=false;result.trace.startSolid=false;result.trace.contents=0;}
}

// Swept/stationary zero-extent world queries over prepared collision meshes. This does not
// implement the original full BSP displacement loader or material binding.
// Borrowed world/meshes must outlive this adapter and remain unchanged while
// it is used; leaf assignments and culling data are derived at construction.
class SourcePreparedWorldRayCandidate {
 const SourceBspBrushOnlyTraceCandidate&world;
 const std::vector<SourceDisplacementRuntimeMeshCandidate>&meshes;
 std::vector<std::vector<uint32_t>>leafDisplacements;
 std::vector<SourceLeafDisplacementCullCandidate>culls;
public:
 SourcePreparedWorldRayCandidate(const SourceBspBrushOnlyTraceCandidate&w,
  const std::vector<SourceDisplacementRuntimeMeshCandidate>&m):world(w),meshes(m),leafDisplacements(w.data.leafBrushes.size()){
  if(m.size()>65535)throw std::runtime_error("too many prepared displacements");
  for(size_t i=0;i<m.size();++i){const auto&b=m[i].tree.expandedBounds;
   SourceLeafDisplacementCullCandidate c{{b[0],b[1],b[2]},{b[3],b[4],b[5]},m[i].contents,uint32_t(i)};
   for(auto leaf:sourceDisplacementLeafAssignmentCandidate(w,c.mins,c.maxs))leafDisplacements.at(leaf).push_back(uint32_t(i));culls.push_back(c);
  }
 }
 const std::vector<std::vector<uint32_t>>&leafRefs()const{return leafDisplacements;}
 void trace(const std::array<float,3>&start,const std::array<float,3>&end,uint32_t mask,SourcePreparedWorldRayStateCandidate&state)const{
  std::array<float,3>delta{};for(size_t a=0;a<3;++a)delta[a]=end[a]-start[a];traceWithDelta(start,end,delta,mask,state);
 }
 // Ray_t semantics: delta is retained even when start+delta rounds differently.
 // Stationary dispatch is deliberately not simulated by this swept entrypoint.
 void traceRay(const std::array<float,3>&start,const std::array<float,3>&delta,uint32_t mask,SourcePreparedWorldRayStateCandidate&state)const{
  if(delta==std::array<float,3>{0,0,0})throw std::runtime_error("stationary world ray requires separate dispatch");
  std::array<float,3>end{};for(size_t a=0;a<3;++a)end[a]=start[a]+delta[a];traceWithDelta(start,end,delta,mask,state);
 }
 // Original 10148030 / 10147F40 stationary dispatch, zero extents only.
 // The nested normal probe deliberately retains the stationary work flag,
 // so displacement visits are not suppressed as they are on swept rays.
 void traceStationaryPoint(const std::array<float,3>&point,uint32_t mask,SourcePreparedWorldRayStateCandidate&state)const{
  if(mask&0x2000)throw std::runtime_error("surface-exclusion mask not supported by prepared world adapter");
  auto result=state;const auto selection=sourceStationaryLeafSelectionCandidate(world,point,{1,1,1});
  std::vector<uint8_t>visited(world.data.brushes.size());bool anyNonSolidLeaf=false;
  for(auto leaf:selection.leaves){
   if(!(world.data.leafContents.at(leaf)&1))anyNonSolidLeaf=true;
   for(auto index:world.data.leafBrushes.at(leaf)){if(visited.at(index))continue;visited[index]=1;if(!(world.data.brushes.at(index).contents&mask))continue;sourceStationaryBrushCandidate(world.data,world.records,index,point,result.trace);if(result.trace.fraction==0)break;}
   const auto&refs=leafDisplacements.at(leaf);
   if(!result.trace.startSolid&&!refs.empty()){
    std::array<float,3>normal{};uint32_t contents=0;
    sourceStationaryDisplacementNormalCandidate(meshes,refs,point,mask,normal,contents);
    stationaryNormalProbe(point,normal,mask,result);
    sourceStationaryDisplacementFinishCandidate(result);
   }
   if(result.trace.allSolid)break;
  }
  if(!anyNonSolidLeaf){result.trace.fraction=0;result.trace.leftSolid=1;result.trace.allSolid=true;result.trace.startSolid=true;}
  state=result;
 }
private:
 void stationaryNormalProbe(const std::array<float,3>&point,const std::array<float,3>&normal,uint32_t mask,SourcePreparedWorldRayStateCandidate&state)const{
  if(world.root!=0)throw std::runtime_error("stationary normal probe requires world root zero");
  std::array<float,3>end{},delta{};
  for(size_t a=0;a<3;++a){const float step=normal[a]*99999.8984375f;end[a]=point[a]+step;delta[a]=end[a]-point[a];}
  auto result=state;result.trace.fraction=1;result.trace.leftSolid=0;result.trace.allSolid=false;result.trace.startSolid=false;result.displacementHit=0;result.trace.surface={};
  // Plane, contents and displacement flags survive this reset in 1014CE00.
  traceWithDelta(point,end,delta,mask,result,false);state=result;
 }
 void traceWithDelta(const std::array<float,3>&start,const std::array<float,3>&end,const std::array<float,3>&delta,uint32_t mask,SourcePreparedWorldRayStateCandidate&state,bool swept=true)const{
  if(mask&0x2000)throw std::runtime_error("surface-exclusion mask not supported by prepared world adapter");
  auto result=state;std::array<float,3>inverse{};
  for(size_t a=0;a<3;++a){inverse[a]=delta[a]==0?std::numeric_limits<float>::max():1.f/delta[a];if(!std::isfinite(start[a])||!std::isfinite(end[a])||!std::isfinite(delta[a])||!std::isfinite(inverse[a]))throw std::runtime_error("invalid prepared world ray");}
  std::vector<uint32_t>contents;for(const auto&b:world.data.brushes)contents.push_back(b.contents);
  std::vector<uint32_t>brushVisits(contents.size()),dispVisits(meshes.size());
  sourceBspLineWalkCandidate(world.planes,world.nodes,world.root,uint32_t(leafDisplacements.size()),start,end,0,1,result.trace.fraction,[&](uint32_t leaf,float,float){
   const auto&brushes=world.data.leafBrushes.at(leaf);
   sourceLeafBrushPolicyCandidate(brushes,contents,brushVisits,1,mask,result.trace,[](uint32_t){return false;},[&](uint32_t brush,auto&s){
    bool entering=false;sourceBrushLineDispatchCandidate(world.data,world.records,brush,start,end,s,&entering,&delta);
    // Original normal-entry branches clear work114; solid/exit paths retain it.
    if(entering)result.displacementHit=0;
   });
   SourceLeafDisplacementStateCandidate phase;phase.fraction=result.trace.fraction;phase.startSolid=result.trace.startSolid;phase.allSolid=result.trace.allSolid;phase.displacementHit=result.displacementHit!=0;std::copy_n(result.trace.plane.begin(),3,phase.normal.begin());
   sourceLeafDisplacementPolicyCandidate(leafDisplacements.at(leaf),culls,dispVisits,1,mask,swept,!brushes.empty(),phase,start,delta,inverse,[&](uint32_t index,auto&s){
    SourceDisplacementWorldRayStateCandidate d;d.ray.fraction=s.fraction;d.ray.normal=s.normal;d.ray.distance=result.trace.plane[3];d.ray.displacementFlags=result.displacementFlags;d.contents=result.trace.contents;d.displacementHit=result.displacementHit;
    sourceDisplacementWorldRayCandidate(meshes.at(index),{0,0},start,delta,inverse,d);
    if(d.surfaceIsDisplacement)result.trace.surface={SourceCollisionSurfaceKindCandidate::Displacement,index};
    s.fraction=d.ray.fraction;s.normal=d.ray.normal;s.displacementHit=d.displacementHit!=0;
    result.trace.plane={d.ray.normal[0],d.ray.normal[1],d.ray.normal[2],d.ray.distance};result.trace.contents=d.contents;result.displacementFlags=d.ray.displacementFlags;result.displacementHit=d.displacementHit;
   });
   result.trace.fraction=phase.fraction;result.trace.startSolid=phase.startSolid;result.trace.allSolid=phase.allSolid;
  });state=result;
 }
};
