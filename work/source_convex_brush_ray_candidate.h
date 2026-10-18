#pragma once
#include "source_bsp_world_collision_data_candidate.h"
#include <limits>
#include "source_collision_surface_candidate.h"
struct SourceConvexBrushRayStateCandidate {
 float fraction=1.0f,leftSolid=0.0f;
 bool startSolid=false,allSolid=false;
 uint32_t contents=0,side=std::numeric_limits<uint32_t>::max();
 SourceCollisionSurfaceCandidate surface;
};
// General (non-optimized-box) line kernel recovered from 10147690.
// General-kernel arithmetic is checked by an original-instruction oracle;
// optimized boxes, surfaces, displacements and world traversal remain separate.
inline void sourceConvexBrushRayCandidate(const SourceBspWorldCollisionDataCandidate&data,uint32_t brushIndex,
 const std::array<float,3>&start,const std::array<float,3>&end,SourceConvexBrushRayStateCandidate&state){
 for(float value:start)if(!std::isfinite(value))throw std::runtime_error("nonfinite brush ray start");
 for(float value:end)if(!std::isfinite(value))throw std::runtime_error("nonfinite brush ray end");
 if(!std::isfinite(state.fraction)||!std::isfinite(state.leftSolid)||state.fraction<0||state.fraction>1||state.leftSolid<0||state.leftSolid>1)throw std::runtime_error("invalid brush trace state");
 const auto&brush=data.brushes.at(brushIndex);if(!brush.count)return;
 if(size_t(brush.first)>data.sides.size()||size_t(brush.count)>data.sides.size()-brush.first)throw std::runtime_error("invalid brush side span");
 float enter=-99999.0f,leave=1.0f;bool startsOutside=false,endsOutside=false;uint32_t entering=std::numeric_limits<uint32_t>::max();
 for(uint32_t i=0;i<brush.count;++i){const uint32_t index=brush.first+i;const auto&side=data.sides[index];if(side.bevel)continue;const auto&plane=data.planes.at(side.plane);
  auto distance=[&](const auto&point){const float y=point[1]*plane.normal[1],x=point[0]*plane.normal[0],z=point[2]*plane.normal[2];const float xy=y+x;const float value=(xy+z)-plane.distance;if(!std::isfinite(value))throw std::runtime_error("brush distance overflow");return value;};
  const float from=distance(start),to=distance(end);
  if(from>0.0f){startsOutside=true;if(to>0.0f)return;}
  else{if(to<=0.0f)continue;endsOutside=true;}
  const float denominator=from-to;
  if(from>to){const float biased=from-0.03125f;const float numerator=biased<0.0f?0.0f:biased;const float fraction=numerator/denominator;if(fraction>enter){enter=fraction;entering=index;}}
  else{const float numerator=from+0.03125f;const float fraction=numerator/denominator;if(fraction<leave)leave=fraction;}
 }
 auto result=state;
 if(!startsOutside||result.leftSolid-enter>0.0f){
  result.startSolid=true;result.contents=brush.contents;
  if(!endsOutside){result.allSolid=true;result.fraction=0.0f;result.leftSolid=1.0f;}
  // Native resets surface only when an exit invalidates the prior fraction;
  // all-solid and shorter-exit branches retain the prior surface and plane.
  else if(leave!=1.0f&&leave>result.leftSolid){result.leftSolid=leave;if(leave>=result.fraction){result.fraction=1.0f;result.surface={};}}
 }else if(leave>enter&&enter> -99999.0f&&result.fraction>enter){result.fraction=enter<0.0f?0.0f:enter;result.contents=brush.contents;result.side=entering;const auto texture=data.sides.at(entering).texture;result.surface=texture<0?SourceCollisionSurfaceCandidate{}:SourceCollisionSurfaceCandidate{SourceCollisionSurfaceKindCandidate::TextureInfo,uint32_t(texture)};}
 if(!std::isfinite(result.fraction)||!std::isfinite(result.leftSolid))throw std::runtime_error("brush fraction overflow");state=result;
}
