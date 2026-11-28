#pragma once
#include "source_displacement_runtime_mesh_candidate.h"

// Full finite bounds predicate10158e80 used by normal selection1014cd40.
// Hull bounds contribute their size, not their midpoint, to mesh expansion.
inline bool sourceStationaryDisplacementNormalBoundsCandidate(const SourceDisplacementRuntimeMeshCandidate& mesh,
 const std::array<float,3>& point,const std::array<float,3>& minimum,
 const std::array<float,3>& maximum,bool isRay){
 const auto& b=mesh.tree.expandedBounds;
 for(size_t a=0;a<3;++a){
  if(!std::isfinite(point[a])||!std::isfinite(b[a])||!std::isfinite(b[a+3])||b[a]>b[a+3])throw std::runtime_error("invalid stationary normal bounds");
  float low=b[a],high=b[a+3];
  if(!isRay){const float extent=(maximum[a]-minimum[a])*.5f;low-=extent;high+=extent;if(!std::isfinite(minimum[a])||!std::isfinite(maximum[a])||minimum[a]>maximum[a]||!std::isfinite(low)||!std::isfinite(high))throw std::runtime_error("stationary normal bounds expansion overflow");}
  if(point[a]>high||low>point[a])return false;
 }
 return true;
}

inline void sourceStationaryDisplacementHullNormalCandidate(
 const std::vector<SourceDisplacementRuntimeMeshCandidate>& meshes,const std::vector<uint32_t>& ordered,
 const std::array<float,3>& point,const std::array<float,3>& minimum,const std::array<float,3>& maximum,
 bool isRay,uint32_t mask,std::array<float,3>& normal,uint32_t& contents){
 if(ordered.empty())return;
 for(auto id:ordered){const auto& m=meshes.at(id);for(float v:m.baseNormal)if(!std::isfinite(v))throw std::runtime_error("invalid stationary base normal");}
 auto selected=meshes.at(ordered.front()).baseNormal;uint32_t selectedContents=1;
 for(auto id:ordered){const auto& m=meshes.at(id);if((m.contents&mask)&&sourceStationaryDisplacementNormalBoundsCandidate(m,point,minimum,maximum,isRay)){selected=m.baseNormal;selectedContents=m.contents;break;}}
 normal=selected;contents=selectedContents;
}
