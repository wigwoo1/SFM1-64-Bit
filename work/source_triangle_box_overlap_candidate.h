#pragma once
#include "source_bounds_plane_side_candidate.h"

// Finite-input triangle/box predicate1014f770. The supplied plane metadata is
// used as stored. In particular, the final plane-side test is not a generic
// inclusive SAT plane test. Preserve the separate edge projections and pairs.
inline bool sourceTriangleBoxOverlapCandidate(const std::array<float,3>& center,
 const std::array<float,3>& extents,const std::array<std::array<float,3>,3>& points,
 const std::array<float,3>& normal,float distance,uint8_t type,uint8_t signs,float tolerance=0.f){
 if(!std::isfinite(tolerance)||tolerance<0.f)throw std::runtime_error("invalid triangle box tolerance");
 std::array<std::array<float,3>,3> v{};
 for(size_t a=0;a<3;++a){
  if(!std::isfinite(center[a])||!std::isfinite(extents[a])||extents[a]<0.f)throw std::runtime_error("invalid triangle box geometry");
  for(size_t p=0;p<3;++p){v[p][a]=points[p][a]-center[a];if(!std::isfinite(points[p][a])||!std::isfinite(v[p][a]))throw std::runtime_error("triangle box translation overflow");}
  float low=v[0][a],high=low;for(size_t p=1;p<3;++p){if(low>v[p][a])low=v[p][a];if(v[p][a]>high)high=v[p][a];}
  const float radius=extents[a]+tolerance;if(!std::isfinite(radius))throw std::runtime_error("triangle box radius overflow");
  if(low>radius||-radius>high)return false;
 }
 auto separated=[](float p,float q,float radius){
  if(!std::isfinite(p)||!std::isfinite(q)||!std::isfinite(radius))throw std::runtime_error("triangle box axis overflow");
  if(q>p)return p>radius||-radius>q;
  return q>radius||-radius>p;
 };
 for(size_t edge=0;edge<3;++edge){
  const size_t next=(edge+1)%3;std::array<float,3> e{};for(size_t a=0;a<3;++a){e[a]=v[next][a]-v[edge][a];if(!std::isfinite(e[a]))throw std::runtime_error("triangle box edge overflow");}
  const size_t x0=0,x1=edge==0?2:1;
  auto projectX=[&](size_t p){const float y=v[p][1]*e[2],z=v[p][2]*e[1];return y-z;};
  const float xy=extents[1]*std::fabs(e[2]),xz=extents[2]*std::fabs(e[1]),xr=xy+xz;
  if(separated(projectX(x0),projectX(x1),xr+tolerance))return false;
  const size_t y0=0,y1=edge==0?2:1;
  auto projectY=[&](size_t p){const float x=e[0]*v[p][2],z=e[2]*v[p][0];return x-z;};
  // Native uses x87 products/accumulation here, storing only the final radius.
  // Each operand is a stored float; accumulate before the final float store.
  const float yr=float(double(std::fabs(e[0]))*extents[2]+double(extents[0])*std::fabs(e[2]));
  if(separated(projectY(y0),projectY(y1),yr+tolerance))return false;
  const size_t z0=edge==0?1:2,z1=edge==1?0:edge==0?2:1;
  auto projectZ=[&](size_t p){const float y=e[1]*v[p][0],x=e[0]*v[p][1];return y-x;};
  const float zy=extents[1]*std::fabs(e[0]),zx=extents[0]*std::fabs(e[1]),zr=zy+zx;
  if(separated(projectZ(z0),projectZ(z1),zr+tolerance))return false;
 }
 std::array<float,3> minimum{},maximum{};for(size_t a=0;a<3;++a){minimum[a]=center[a]-extents[a];maximum[a]=extents[a]+center[a];}
 return sourceBoundsPlaneSideCandidate(minimum,maximum,normal,distance,type,signs)==3;
}
