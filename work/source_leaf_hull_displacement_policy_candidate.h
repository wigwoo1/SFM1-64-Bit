#pragma once
#include "source_leaf_displacement_policy_candidate.h"

// Displacement phase of non-ray leaf10148540. Brush processing is separate.
// Its filter/visit/finish ordering matches the point phase, but bounds passed
// to1014e790 must first expand by the actual hull extents, in float precision.
template<class State,class TraceDisplacement>
void sourceLeafHullDisplacementPolicyCandidate(const std::vector<uint32_t>& ordered,
 const std::vector<SourceLeafDisplacementCullCandidate>& culls,std::vector<uint32_t>& visited,
 uint32_t generation,uint32_t mask,bool swept,bool leafHasBrushes,State& state,
 const std::array<float,3>& start,const std::array<float,3>& delta,
 const std::array<float,3>& inverse,const std::array<float,3>& extents,TraceDisplacement trace) {
 if((leafHasBrushes&&state.startSolid)||ordered.empty())return;
 for(float value:extents)if(!std::isfinite(value)||value<0)throw std::runtime_error("invalid leaf hull extents");
 for(uint32_t index:ordered){
  if(index>=culls.size()||culls[index].visitIndex>=visited.size())throw std::runtime_error("invalid hull leaf displacement reference");
  const auto& c=culls[index];for(size_t a=0;a<3;++a)if(!std::isfinite(c.mins[a])||!std::isfinite(c.maxs[a])||c.mins[a]>c.maxs[a])throw std::runtime_error("invalid hull displacement cull bounds");
 }
 auto staged=state;auto stagedVisits=visited;
 for(uint32_t index:ordered){
  const auto& c=culls[index];if(!(c.contents&mask))continue;
  if(swept){auto& mark=stagedVisits[c.visitIndex];if(mark==generation)continue;mark=generation;}
  std::array<float,3> low{},high{};
  for(size_t a=0;a<3;++a){low[a]=c.mins[a]-extents[a];high[a]=c.maxs[a]+extents[a];}
  if(!sourceDisplacementBoundsGateCandidate(low,high,start,delta,inverse,.03125f))continue;
  trace(index,staged);if(staged.fraction==0.f)break;
 }
 if(staged.displacementHit){const float y=staged.normal[1]*delta[1],x=staged.normal[0]*delta[0],z=staged.normal[2]*delta[2],xy=y+x;if(xy+z>0.f){staged.allSolid=true;staged.startSolid=true;}}
 state=staged;visited=std::move(stagedVisits);
}
