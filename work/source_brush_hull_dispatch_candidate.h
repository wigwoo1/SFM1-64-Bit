#pragma once
#include "source_bsp_collision_records_candidate.h"
#include "source_convex_brush_hull_candidate.h"
#include "source_optimized_box_line_ray_candidate.h"

// Per-brush hull dispatch. Host owns masks, deduplication, traversal and
// displacement/scene contacts. Retained delta is required for Ray_t semantics.
inline void sourceBrushHullDispatchCandidate(const SourceBspWorldCollisionDataCandidate& data,
 const std::vector<SourceBspCollisionRecordCandidate>& records,uint32_t index,
 const std::array<float,3>& start,const std::array<float,3>& end,
 const std::array<float,3>& delta,const std::array<float,3>& extents,
 SourceOptimizedBoxLineStateCandidate& state,bool* enteringPlaneHit=nullptr) {
 if(records.size()!=data.brushes.size())throw std::runtime_error("hull collision record count mismatch");
 const auto& brush=data.brushes.at(index);const auto& record=records.at(index);
 if(record.optimized){
  std::array<float,3> mins{},maxs{};std::memcpy(mins.data(),record.box.data(),12);std::memcpy(maxs.data(),record.box.data()+16,12);
  bool entering=false;sourceOptimizedBoxLineRayCandidate(mins,maxs,start,end,brush.contents,state,&entering,&delta,&extents);
  if(entering){if(state.surface.kind!=SourceCollisionSurfaceKindCandidate::BoxFace||state.surface.index>5)throw std::runtime_error("invalid hull box surface event");uint16_t surface;std::memcpy(&surface,record.box.data()+32+2*state.surface.index,2);state.surface=surface==65535?SourceCollisionSurfaceCandidate{}:SourceCollisionSurfaceCandidate{SourceCollisionSurfaceKindCandidate::RuntimeSurface,surface};if(enteringPlaneHit)*enteringPlaneHit=true;}
  return;
 }
 SourceConvexBrushRayStateCandidate general;general.fraction=state.fraction;general.leftSolid=state.leftSolid;general.startSolid=state.startSolid;general.allSolid=state.allSolid;general.contents=state.contents;general.surface=state.surface;
 sourceConvexBrushHullCandidate(data,index,start,end,extents,general);
 auto result=state;result.fraction=general.fraction;result.leftSolid=general.leftSolid;result.startSolid=general.startSolid;result.allSolid=general.allSolid;result.contents=general.contents;result.surface=general.surface;
 if(general.side!=UINT32_MAX){const auto& p=data.planes.at(data.sides.at(general.side).plane);result.plane={p.normal[0],p.normal[1],p.normal[2],p.distance};result.planeTags=p.tags();if(enteringPlaneHit)*enteringPlaneHit=true;}
 state=result;
}
