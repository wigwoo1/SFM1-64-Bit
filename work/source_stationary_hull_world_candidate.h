#pragma once
#include "source_stationary_leaf_selection_candidate.h"
#include "source_stationary_hull_leaf_candidate.h"

struct SourceStationaryHullWorldOutcomeCandidate {
 SourceStationaryLeafSelectionCandidate selection;
 bool visitedNonSolid=false;
};

// Full prepared world policy10148030. A selected leaf is processed even if its
// solid bit is set; that bit only contributes to the final visited-leaf fallback.
// Check allSolid after a leaf call, not before the first call.
template<class VisitLeaf>
bool sourceStationaryHullWorldPolicyCandidate(const std::vector<uint32_t>& ordered,
 const std::vector<uint32_t>& contents,SourcePreparedWorldHullStateCandidate& state,VisitLeaf visitLeaf){
 for(auto leaf:ordered)if(leaf>=contents.size())throw std::runtime_error("stationary world leaf reference");
 auto result=state;bool visitedNonSolid=false;
 for(auto leaf:ordered){
  if(!(contents[leaf]&1u))visitedNonSolid=true;
  visitLeaf(leaf,result);
  if(result.trace.allSolid)break;
 }
 if(!visitedNonSolid){result.trace.allSolid=true;result.trace.startSolid=true;result.trace.fraction=0.f;result.trace.leftSolid=1.f;}
 state=std::move(result);return visitedNonSolid;
}

template<class TraceHull>
SourceStationaryHullWorldOutcomeCandidate sourceStationaryHullWorldCandidate(
 const SourceBspBrushOnlyTraceCandidate& world,const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const std::vector<std::vector<uint32_t>>& leafDisplacements,const std::vector<SourceLeafDisplacementCullCandidate>& culls,
 int32_t root,const std::array<float,3>& point,const std::array<float,3>& extents,
 std::vector<uint32_t>& brushVisits,std::vector<uint32_t>& displacementVisits,uint32_t generation,
 uint32_t mask,const std::array<uint16_t,2>& defaultSurface,SourcePreparedWorldHullStateCandidate& state,
 TraceHull traceHull){
 if(world.data.leafContents.size()!=world.data.leafBrushes.size()||leafDisplacements.size()!=world.data.leafBrushes.size())throw std::runtime_error("stationary world leaf table coverage");
 std::array<float,3> selectionExtents{},minimum{};
 for(size_t a=0;a<3;++a){selectionExtents[a]=extents[a]+1.f;minimum[a]=-extents[a];if(!std::isfinite(extents[a])||extents[a]<0.f||!std::isfinite(selectionExtents[a]))throw std::runtime_error("invalid stationary world extents");}
 SourceStationaryHullWorldOutcomeCandidate outcome;outcome.selection=sourceStationaryLeafSelectionAtRootCandidate(world,root,point,selectionExtents,1024);
 auto result=state;auto brushMarks=brushVisits;auto displacementMarks=displacementVisits;
 outcome.visitedNonSolid=sourceStationaryHullWorldPolicyCandidate(outcome.selection.leaves,world.data.leafContents,result,
  [&](uint32_t leaf,auto& output){sourceStationaryHullLeafCandidate(world,meshes,culls,world.data.leafBrushes.at(leaf),leafDisplacements.at(leaf),brushMarks,displacementMarks,generation,point,minimum,extents,mask,defaultSurface,output,traceHull);});
 state=std::move(result);brushVisits=std::move(brushMarks);displacementVisits=std::move(displacementMarks);return outcome;
}
