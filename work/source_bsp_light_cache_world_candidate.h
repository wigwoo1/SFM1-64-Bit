#pragma once
#include "source_light_cache_position_candidate.h"
#include "source_world_ray_trace_candidate.h"

// Real checked BSP dependencies for the recovered cache-position policy.
// This computes a cache sample position, not a complete model-lighting result.
// All borrowed collision objects must describe the same BSP and remain alive
// and unchanged. This adapter owns its point-leaf/constraint tree.
class SourceBspLightCacheWorldCandidate {
 SourceBspPointLeafCandidate tree;
 const SourceBspBrushOnlyTraceCandidate&collision;
 const SourcePreparedWorldRayCandidate&queries;
public:
 SourceBspLightCacheWorldCandidate(const std::vector<uint8_t>&b,
  const SourceBspBrushOnlyTraceCandidate&world,const SourcePreparedWorldRayCandidate&prepared)
  :tree(b),collision(world),queries(prepared){
  if(tree.worldRoot()!=0||world.root!=0||tree.leafCount()!=world.data.leafContents.size()||tree.nodeCount()!=world.nodes.size()||tree.planeCount()!=world.planes.size()||prepared.leafRefs().size()!=tree.leafCount())throw std::runtime_error("cache world spatial tables mismatch");
 }
 int32_t leafAt(const std::array<float,3>&point)const{return int32_t(tree.leafAt(point));}
 uint32_t contents(int32_t leaf)const{if(leaf<0)throw std::runtime_error("negative cache leaf");return collision.data.leafContents.at(size_t(leaf));}
 void constrainToStartLeaf(const std::array<float,3>&start,float tolerance,std::array<float,3>&target)const{target=tree.constrainToStartLeaf(start,tolerance,target);}
 SourceLightCacheTraceCandidate trace(const std::array<float,3>&start,const std::array<float,3>&end,uint32_t mask)const{
  std::array<float,3>delta{};for(size_t a=0;a<3;++a)delta[a]=end[a]-start[a];const auto result=sourceWorldRayTraceCandidate(queries,start,delta,mask);return {result.state.trace.fraction,result.state.trace.startSolid};
 }
 std::array<float,3>position(const std::array<float,3>&point,int32_t inputLeaf)const{
  (void)contents(inputLeaf);return sourceLightCachePositionCandidate(point,inputLeaf,*this);
 }
 std::array<float,3>position(const std::array<float,3>&point)const{return position(point,leafAt(point));}
};
