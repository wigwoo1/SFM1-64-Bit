#pragma once
#include "source_convex_brush_ray_candidate.h"

// General convex hull kernel, engine10147920's non-optimized branch. Includes
// bevels and uses the hull-specific start-solid policy. Caller owns traversal,
// masking, optimized-box selection and runtime surface/plane binding.
inline void sourceConvexBrushHullCandidate(const SourceBspWorldCollisionDataCandidate& data,uint32_t brushIndex,
 const std::array<float,3>& start,const std::array<float,3>& end,
 const std::array<float,3>& extents,SourceConvexBrushRayStateCandidate& state) {
 for(size_t a=0;a<3;++a)if(!std::isfinite(start[a])||!std::isfinite(end[a])||!std::isfinite(extents[a])||extents[a]<0)throw std::runtime_error("invalid convex hull geometry");
 if(!std::isfinite(state.fraction)||!std::isfinite(state.leftSolid)||state.fraction<0||state.fraction>1||state.leftSolid<0||state.leftSolid>1)throw std::runtime_error("invalid convex hull trace state");
 const auto& brush=data.brushes.at(brushIndex);if(!brush.count)return;
 if(size_t(brush.first)>data.sides.size()||size_t(brush.count)>data.sides.size()-brush.first)throw std::runtime_error("invalid hull brush sides");
 float enter=-99999.f,leave=1.f;bool startsOutside=false,endsOutside=false;uint32_t entering=UINT32_MAX;
 for(uint32_t i=0;i<brush.count;++i) {
  const auto index=brush.first+i;const auto& side=data.sides[index];const auto& plane=data.planes.at(side.plane);
  const double y=std::abs(double(extents[1])*plane.normal[1]),x=std::abs(double(extents[0])*plane.normal[0]),z=std::abs(double(extents[2])*plane.normal[2]);
  const float expandedDistance=float(((y+x)+z)+plane.distance);
  if(!std::isfinite(expandedDistance))throw std::runtime_error("hull plane expansion overflow");
  auto distance=[&](const auto& point){const float py=point[1]*plane.normal[1],px=point[0]*plane.normal[0],pz=point[2]*plane.normal[2];const float xy=py+px;const float v=(xy+pz)-expandedDistance;if(!std::isfinite(v))throw std::runtime_error("hull brush distance overflow");return v;};
  const float from=distance(start),to=distance(end);
  if(from>0){startsOutside=true;if(to>0)return;}else{if(to<=0)continue;endsOutside=true;}
  const float denominator=from-to;
  if(from>to){const float biased=from-.03125f,numerator=biased<0?0.f:biased,fraction=numerator/denominator;if(fraction>enter){enter=fraction;entering=index;}}
  else{const float numerator=from+.03125f,fraction=numerator/denominator;if(fraction<leave)leave=fraction;}
 }
 auto result=state;
 if(!startsOutside){result.startSolid=true;result.contents=brush.contents;if(!endsOutside){result.allSolid=true;result.fraction=0;result.leftSolid=1;}else if(leave!=1&&leave>result.leftSolid){result.leftSolid=leave;if(leave>=result.fraction){result.fraction=1;result.surface={};}}}
 else if(leave>enter&&enter>-99999.f&&result.fraction>enter){result.fraction=enter<0?0.f:enter;result.contents=brush.contents;result.side=entering;const auto texture=data.sides.at(entering).texture;result.surface=texture<0?SourceCollisionSurfaceCandidate{}:SourceCollisionSurfaceCandidate{SourceCollisionSurfaceKindCandidate::TextureInfo,uint32_t(texture)};}
 if(!std::isfinite(result.fraction)||!std::isfinite(result.leftSolid))throw std::runtime_error("hull brush fraction overflow");state=result;
}
