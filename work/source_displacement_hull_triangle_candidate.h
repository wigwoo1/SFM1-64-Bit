#pragma once
#include "source_displacement_collision_triangle_candidate.h"

struct SourceDisplacementHullClipCandidate {
 float enter=-99999.8984375f,exit=1;
 std::array<float,3> normal{};float distance=0;
};
struct SourceDisplacementHullTraceCandidate {
 float fraction=1;std::array<float,3> normal{99,98,97};float distance=96;
 uint16_t displacementFlags=0xbeef;
};
// Shared native clip10158cb0. Distances are already hull-expanded; retained
// output plane is supplied separately. Equal/touching endpoints are significant.
inline bool sourceDisplacementHullClipCandidate(float from,float to,
 const std::array<float,3>& normal,float distance,SourceDisplacementHullClipCandidate& clip) {
 if(from>0&&to>0)return false;if(from<0&&to<0)return true;
 const float denominator=from-to;const bool equal=denominator==0;
 if(from>=0&&to<=0){const float numerator=from-.03125f,entry=equal?0.f:numerator/denominator;if(entry>clip.enter){clip.enter=entry;clip.normal=normal;clip.distance=distance;}}
 else{const float numerator=from+.03125f,exit=equal?0.f:numerator/denominator;if(clip.exit>exit)clip.exit=exit;}
 return true;
}

// Actual triangle contact1015abb0, for supplied cache planes/indices. Cache
// generation/merging is separate. Each cached triple stores plane distance in
// its omitted-axis component; other two components are normal coefficients.
inline bool sourceDisplacementHullTriangleCandidate(const std::vector<std::array<float,3>>& vertices,
 const SourceDisplacementCollisionTriangleCandidate& triangle,
 const std::vector<std::array<float,3>>& planes,const std::array<uint16_t,9>& indices,
 const std::array<float,3>& start,const std::array<float,3>& delta,
 const std::array<float,3>& extents,const std::array<float,3>& normalizedDelta,
 SourceDisplacementHullTraceCandidate& trace) {
 for(size_t a=0;a<3;++a)if(!std::isfinite(start[a])||!std::isfinite(delta[a])||!std::isfinite(extents[a])||extents[a]<0||!std::isfinite(normalizedDelta[a])||!std::isfinite(triangle.normal[a]))throw std::runtime_error("invalid displacement hull contact input");
 if(!std::isfinite(triangle.distance)||!std::isfinite(trace.fraction)||trace.fraction<0||trace.fraction>1)throw std::runtime_error("invalid displacement hull contact state");
 // The native facing gate uses raw delta, not the supplied normalized direction.
 const float facingY=delta[1]*triangle.normal[1],facingX=delta[0]*triangle.normal[0],facingZ=delta[2]*triangle.normal[2],facingXY=facingY+facingX;
 if(facingXY+facingZ>.03125f)return false;
 SourceDisplacementHullClipCandidate clip;
 for(int axis=2;axis>=0;--axis){const auto packed=triangle.packedVertices[size_t(axis)];const auto lowIndex=(packed>>9)&3u,highIndex=(packed>>11)&3u;if(lowIndex>2||highIndex>2)throw std::runtime_error("invalid triangle extrema selector");
  const auto lowVertex=triangle.packedVertices[lowIndex]&511u,highVertex=triangle.packedVertices[highIndex]&511u;
  const float low=vertices.at(lowVertex)[size_t(axis)],high=vertices.at(highVertex)[size_t(axis)];
  std::array<float,3> normal{};normal[size_t(axis)]=-1;
  const float lowExpanded=low-extents[size_t(axis)],fromLow=lowExpanded-start[size_t(axis)],toLow=fromLow-delta[size_t(axis)];
  if(!sourceDisplacementHullClipCandidate(fromLow,toLow,normal,low,clip))return false;
  normal[size_t(axis)]=1;const float highExpanded=high+extents[size_t(axis)],fromHigh=start[size_t(axis)]-highExpanded,toHigh=fromHigh+delta[size_t(axis)];
  if(!sourceDisplacementHullClipCandidate(fromHigh,toHigh,normal,high,clip))return false;
 }
 for(size_t family=0;family<3;++family)for(size_t edge=0;edge<3;++edge){const uint16_t id=indices[family*3+edge];if(id==65535)continue;auto plane=planes.at(id&32767u);if(id&32768u)for(auto& v:plane)v=-v;
  const size_t a=family==0?1:family==1?2:1,b=family==0?2:0;std::array<float,3> normal=plane;normal[family]=0;
  const float supportA=normal[a]<0?extents[a]:-extents[a],supportB=normal[b]<0?extents[b]:-extents[b];
  const float sa=supportA*normal[a],sb=supportB*normal[b],support=sb+sa,expanded=plane[family]-support;
  const float pa=start[a]*normal[a],pb=start[b]*normal[b],from=(pa+pb)-expanded;
  const float endA=start[a]+delta[a],endB=start[b]+delta[b],ea=endA*normal[a],eb=endB*normal[b],to=(ea+eb)-expanded;
  if(!sourceDisplacementHullClipCandidate(from,to,normal,plane[family],clip))return false;
 }
 const auto& normal=triangle.normal;std::array<float,3> support{};for(size_t a=0;a<3;++a)support[a]=normal[a]<0?extents[a]:-extents[a];
 const float sx=normal[0]*support[0],sy=normal[1]*support[1],sz=normal[2]*support[2],sxy=sy+sx,expanded=triangle.distance-(sxy+sz);
 const float px=normal[0]*start[0],py=normal[1]*start[1],pz=normal[2]*start[2],pxy=py+px,from=(pxy+pz)-expanded;
 const float ex=start[0]+delta[0],ey=start[1]+delta[1],ez=start[2]+delta[2],dx=normal[0]*ex,dy=normal[1]*ey,dz=normal[2]*ez,dxy=dx+dy,to=(dxy+dz)-expanded;
 if(!sourceDisplacementHullClipCandidate(from,to,normal,triangle.distance,clip))return false;
 if(!(clip.exit>clip.enter||std::abs(double(clip.enter)-clip.exit)<double(.001f))||clip.enter==-99999.8984375f||!(trace.fraction>clip.enter))return false;
 auto staged=trace;staged.fraction=clip.enter<0?0.f:clip.enter;staged.normal=clip.normal;staged.distance=clip.distance;staged.displacementFlags=uint16_t((triangle.flags>>6)&31u);trace=staged;return true;
}
