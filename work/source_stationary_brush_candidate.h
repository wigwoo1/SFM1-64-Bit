#pragma once
#include "source_brush_line_dispatch_candidate.h"
// Zero-extent stationary brush primitive, recovered from10146C30.
// Does not select surrounding leaves or implement stationary world fallback.
inline void sourceStationaryBrushCandidate(const SourceBspWorldCollisionDataCandidate&data,
 const std::vector<SourceBspCollisionRecordCandidate>&records,uint32_t index,
 const std::array<float,3>&point,SourceOptimizedBoxLineStateCandidate&state){
 for(float v:point)if(!std::isfinite(v))throw std::runtime_error("nonfinite stationary point");
 if(records.size()!=data.brushes.size())throw std::runtime_error("stationary record mismatch");
 const auto&brush=data.brushes.at(index);const auto&record=records.at(index);
 if(record.optimized){for(size_t axis=0;axis<3;++axis){float low,high;std::memcpy(&low,record.box.data()+4*axis,4);std::memcpy(&high,record.box.data()+16+4*axis,4);if(point[axis]<low||point[axis]>high)return;}}
 else{
  if(!brush.count)return;
  if(brush.first>data.sides.size()||brush.count>data.sides.size()-brush.first)throw std::runtime_error("stationary side span");
  for(size_t i=0;i<brush.count;++i){const auto&p=data.planes.at(data.sides.at(brush.first+i).plane);const float x=point[0]*p.normal[0],y=point[1]*p.normal[1],z=point[2]*p.normal[2];const float xy=x+y;const float distance=(xy+z)-p.distance;if(!std::isfinite(distance))throw std::runtime_error("stationary distance overflow");if(distance>0)return;}
 }
 auto result=state;result.startSolid=true;result.allSolid=true;result.fraction=0;result.leftSolid=1;result.contents=brush.contents;state=result;
}
