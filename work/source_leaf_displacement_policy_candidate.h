#pragma once
#include "source_displacement_bounds_gate_candidate.h"
#include <cstdint>
#include <vector>

struct SourceLeafDisplacementCullCandidate {
 std::array<float,3>mins{},maxs{};
 uint32_t contents=0,visitIndex=0;
};
struct SourceLeafDisplacementStateCandidate {
 float fraction=1;
 bool startSolid=false,allSolid=false,displacementHit=false;
 std::array<float,3>normal{};
};

// Displacement phase of 10148270, after the brush phase. Its contents test
// precedes generation marking. Callback owns actual mesh hit/surface updates.
template<class State,class TraceDisplacement>
void sourceLeafDisplacementPolicyCandidate(const std::vector<uint32_t>&ordered,
 const std::vector<SourceLeafDisplacementCullCandidate>&culls,std::vector<uint32_t>&visited,
 uint32_t generation,uint32_t mask,bool swept,bool leafHasBrushes,State&state,const std::array<float,3>&start,
 const std::array<float,3>&delta,const std::array<float,3>&inverse,TraceDisplacement trace){
 // Original zero-brush branch jumps directly to the displacement count,
 // bypassing the startSolid gate located after the nonempty brush loop.
 if((leafHasBrushes&&state.startSolid)||ordered.empty())return;
 for(uint32_t index:ordered){if(index>=culls.size()||culls[index].visitIndex>=visited.size())throw std::runtime_error("invalid leaf displacement reference");const auto&c=culls[index];for(size_t a=0;a<3;++a)if(!std::isfinite(c.mins[a])||!std::isfinite(c.maxs[a])||c.mins[a]>c.maxs[a])throw std::runtime_error("invalid displacement cull bounds");}
 auto staged=state;auto stagedVisits=visited;
 for(uint32_t index:ordered){
  const auto&c=culls[index];if(!(c.contents&mask))continue;
  if(swept){auto&mark=stagedVisits[c.visitIndex];if(mark==generation)continue;mark=generation;}
  if(!sourceDisplacementBoundsGateCandidate(c.mins,c.maxs,start,delta,inverse,.03125f))continue;
  trace(index,staged);if(staged.fraction==0.f)break;
 }
 // Original 1014CFA0 runs even if every record was masked/already visited.
 if(staged.displacementHit){const float y=staged.normal[1]*delta[1],x=staged.normal[0]*delta[0],z=staged.normal[2]*delta[2];const float xy=y+x,dot=xy+z;if(dot>0.f){staged.allSolid=true;staged.startSolid=true;}}
 state=staged;visited=std::move(stagedVisits);
}
