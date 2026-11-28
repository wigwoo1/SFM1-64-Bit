#pragma once
#include "source_brush_line_dispatch_candidate.h"

// Nonzero-hull branch of stationary brush primitive10146c30. The bounds are
// relative to center (native work+18/+24); callers usually supply -/+extents.
// This primitive does not apply contents masks, select leaves or test terrain.
inline void sourceStationaryBrushHullCandidate(const SourceBspWorldCollisionDataCandidate& data,
 const std::vector<SourceBspCollisionRecordCandidate>& records,uint32_t index,
 const std::array<float,3>& center,const std::array<float,3>& minimum,
 const std::array<float,3>& maximum,SourceOptimizedBoxLineStateCandidate& state){
 if(records.size()!=data.brushes.size())throw std::runtime_error("stationary hull record coverage");
 for(size_t a=0;a<3;++a)if(!std::isfinite(center[a])||!std::isfinite(minimum[a])||!std::isfinite(maximum[a])||minimum[a]>maximum[a])throw std::runtime_error("invalid stationary hull bounds");
 const auto& brush=data.brushes.at(index);const auto& record=records.at(index);
 if(record.optimized){
  for(size_t a=0;a<3;++a){
   float low,high;std::memcpy(&low,record.box.data()+4*a,4);std::memcpy(&high,record.box.data()+16+4*a,4);
   const float hullLow=minimum[a]+center[a],hullHigh=maximum[a]+center[a];
   if(!std::isfinite(low)||!std::isfinite(high)||low>high||!std::isfinite(hullLow)||!std::isfinite(hullHigh))throw std::runtime_error("stationary hull box arithmetic overflow");
   if(std::min(high,hullHigh)<std::max(low,hullLow))return;
  }
 }else{
  if(!brush.count)return;
  if(brush.first>data.sides.size()||brush.count>data.sides.size()-brush.first)throw std::runtime_error("stationary hull side span");
  for(size_t i=0;i<brush.count;++i){
   const auto& p=data.planes.at(data.sides.at(brush.first+i).plane);
   std::array<float,3> corner{};
   for(size_t a=0;a<3;++a){if(!std::isfinite(p.normal[a]))throw std::runtime_error("invalid stationary hull normal");corner[a]=p.normal[a]<0?maximum[a]:minimum[a];}
   const float x=center[0]*p.normal[0],y=center[1]*p.normal[1],z=center[2]*p.normal[2],xy=x+y;
   const float ox=p.normal[0]*corner[0],oy=p.normal[1]*corner[1],oz=p.normal[2]*corner[2],oxy=ox+oy;
   const float shiftedDistance=p.distance-(oxy+oz);
   const float separation=(xy+z)-shiftedDistance;
   if(!std::isfinite(p.distance)||!std::isfinite(separation))throw std::runtime_error("stationary hull plane arithmetic overflow");
   if(separation>0.f)return;
  }
 }
 auto result=state;result.allSolid=true;result.startSolid=true;result.fraction=0.f;
 result.leftSolid=1.f;result.contents=brush.contents;state=result;
}
