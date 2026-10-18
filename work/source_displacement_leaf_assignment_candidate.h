#pragma once
#include "source_bsp_brush_only_trace_candidate.h"
#include "source_bounds_plane_side_candidate.h"
#include <deque>
// Bounds assignment only. Caller must supply actual reconstructed runtime bounds.
inline std::vector<uint32_t> sourceDisplacementLeafAssignmentCandidate(const SourceBspBrushOnlyTraceCandidate&world,
 const std::array<float,3>&mins,const std::array<float,3>&maxs){
 for(size_t i=0;i<3;++i)if(!std::isfinite(mins[i])||!std::isfinite(maxs[i])||mins[i]>maxs[i])throw std::runtime_error("invalid displacement bounds");
 std::deque<int32_t>pending{world.root};std::vector<uint32_t>leaves;
 while(!pending.empty()){int32_t cursor=pending.front();pending.pop_front();if(cursor<0){leaves.push_back(uint32_t(-1-int64_t(cursor)));continue;}
  const auto&n=world.nodes.at(size_t(cursor));const auto&p=world.planes.at(n.plane);uint8_t signs=0;for(unsigned i=0;i<3;++i)if(p.normal[i]<0)signs|=uint8_t(1u<<i);
  const auto sides=sourceBoundsPlaneSideCandidate(mins,maxs,p.normal,p.distance,p.type,signs);
  for(unsigned side=0;side<2;++side)if(sides&(1u<<side)){if(pending.size()>=1023)throw std::runtime_error("native displacement leaf queue limit exceeded");pending.push_back(n.children[side]);}
 }return leaves;
}
