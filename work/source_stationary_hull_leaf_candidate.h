#pragma once
#include "source_stationary_brush_hull_candidate.h"
#include "source_stationary_displacement_hull_policy_candidate.h"

// Full prepared non-ray leaf policy10147f40. Unlike the swept brush policy,
// there is no any-surface exclusion. Mark before contents filtering; fraction
// zero terminates only after an actual brush call. A prior startSolid suppresses
// terrain even when the brush list is empty.
template<class TraceBrush,class TraceDisplacements>
void sourceStationaryHullLeafPolicyCandidate(const std::vector<uint32_t>& ordered,
 const std::vector<uint32_t>& contents,std::vector<uint32_t>& visited,uint32_t generation,
 uint32_t mask,bool hasDisplacements,SourcePreparedWorldHullStateCandidate& state,
 TraceBrush traceBrush,TraceDisplacements traceDisplacements){
 if(!generation||visited.size()!=contents.size())throw std::runtime_error("stationary leaf brush visit coverage");
 for(auto index:ordered)if(index>=contents.size())throw std::runtime_error("stationary leaf brush reference");
 auto result=state;auto visits=visited;
 for(auto index:ordered){
  if(visits[index]==generation)continue;visits[index]=generation;
  if(!(contents[index]&mask))continue;
  traceBrush(index,result);
  if(result.trace.fraction==0.f){state=std::move(result);visited=std::move(visits);return;}
 }
 if(!result.trace.startSolid&&hasDisplacements)traceDisplacements(result);
 state=std::move(result);visited=std::move(visits);
}

template<class TraceHull>
void sourceStationaryHullLeafCandidate(const SourceBspBrushOnlyTraceCandidate& world,
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,
 const std::vector<SourceLeafDisplacementCullCandidate>& culls,
 const std::vector<uint32_t>& brushes,const std::vector<uint32_t>& displacements,
 std::vector<uint32_t>& brushVisits,std::vector<uint32_t>& displacementVisits,uint32_t generation,
 const std::array<float,3>& point,const std::array<float,3>& minimum,const std::array<float,3>& maximum,
 uint32_t mask,const std::array<uint16_t,2>& defaultSurface,SourcePreparedWorldHullStateCandidate& state,
 TraceHull traceHull){
 auto result=state;auto brushMarks=brushVisits;auto displacementMarks=displacementVisits;
 std::vector<uint32_t> contents;contents.reserve(world.data.brushes.size());
 for(const auto& brush:world.data.brushes)contents.push_back(brush.contents);
 sourceStationaryHullLeafPolicyCandidate(brushes,contents,brushMarks,generation,mask,!displacements.empty(),result,
  [&](uint32_t index,auto& output){sourceStationaryBrushHullCandidate(world.data,world.records,index,point,minimum,maximum,output.trace);},
  [&](auto& output){sourceStationaryDisplacementHullCandidate(meshes,culls,displacements,displacementMarks,generation,point,minimum,maximum,mask,defaultSurface,output,traceHull);});
 state=std::move(result);brushVisits=std::move(brushMarks);displacementVisits=std::move(displacementMarks);
}
