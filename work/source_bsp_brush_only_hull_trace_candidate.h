#pragma once
#include "source_bsp_brush_only_trace_candidate.h"
#include "source_bsp_hull_walk_candidate.h"
#include "source_brush_hull_dispatch_candidate.h"

// Swept hull BRUSH phase only. Displacements, entities, static props and
// stationary dispatch are excluded. Host supplies the any-surface0x80 rule.
template<class SurfaceExcludes>
void sourceBspBrushOnlyHullTraceCandidate(const SourceBspBrushOnlyTraceCandidate& world,
 const std::array<float,3>& center,const std::array<float,3>& delta,
 const std::array<float,3>& extents,uint32_t mask,SourceOptimizedBoxLineStateCandidate& state,
 SurfaceExcludes surfaceExcludes) {
 auto result=state;std::array<float,3> end{};for(size_t a=0;a<3;++a)end[a]=center[a]+delta[a];
 std::vector<uint32_t> contents;contents.reserve(world.data.brushes.size());for(const auto& b:world.data.brushes)contents.push_back(b.contents);
 std::vector<uint32_t> visited(contents.size());
 sourceBspHullWalkCandidate(world.planes,world.nodes,world.root,uint32_t(world.data.leafBrushes.size()),center,end,extents,0,1,result.fraction,
  [&](uint32_t leaf,float,float){
   sourceLeafBrushPolicyCandidate(world.data.leafBrushes.at(leaf),contents,visited,1,mask,result,surfaceExcludes,
    [&](uint32_t brush,auto& hit){sourceBrushHullDispatchCandidate(world.data,world.records,brush,center,end,delta,extents,hit);});
  });
 state=result;
}
