#pragma once
#include "source_bsp_brush_only_trace_candidate.h"
#include <deque>
struct SourceStationaryLeafSelectionCandidate{int32_t topNode=-1;std::vector<uint32_t>leaves;};
// Checked-world input; recovered FIFO-deferred front children, back first.
// Extents here already include the stationary caller's extra one unit.
inline SourceStationaryLeafSelectionCandidate sourceStationaryLeafSelectionCandidate(
 const SourceBspBrushOnlyTraceCandidate&world,const std::array<float,3>&center,
 const std::array<float,3>&extent,size_t capacity=1024){
 for(float v:center)if(!std::isfinite(v))throw std::runtime_error("invalid leaf center");for(float v:extent)if(!std::isfinite(v)||v<0)throw std::runtime_error("invalid leaf extent");
 SourceStationaryLeafSelectionCandidate result;std::deque<int32_t>pending;int32_t cursor=world.root,last=-1;
 for(;;){
  if(cursor<0){if(result.topNode==-1)result.topNode=last;if(result.leaves.size()<capacity)result.leaves.push_back(uint32_t(-1-int64_t(cursor)));if(pending.empty())break;cursor=pending.front();pending.pop_front();continue;}
  const auto&n=world.nodes.at(size_t(cursor));const auto&p=world.planes.at(n.plane);last=cursor;
  const float y=center[1]*p.normal[1],x=center[0]*p.normal[0],z=center[2]*p.normal[2];const float xy=y+x;const float distance=(xy+z)-p.distance;
  const double ry=std::fabs(double(extent[1])*p.normal[1]),rx=std::fabs(double(extent[0])*p.normal[0]),rz=std::fabs(double(extent[2])*p.normal[2]);const float radius=float((ry+rx)+rz);
  if(!std::isfinite(distance)||!std::isfinite(radius))throw std::runtime_error("leaf selection arithmetic overflow");
  if(distance>=radius){cursor=n.children[0];continue;}if(distance< -radius){cursor=n.children[1];continue;}
  if(result.topNode==-1)result.topNode=cursor;
  if(pending.size()>=1023)throw std::runtime_error("native stationary leaf queue limit exceeded");pending.push_back(n.children[0]);cursor=n.children[1];
 }return result;
}
