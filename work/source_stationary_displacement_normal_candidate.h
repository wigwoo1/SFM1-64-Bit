#pragma once
#include "source_displacement_runtime_mesh_candidate.h"

// Zero-extent branch of original 1014CD40. No triangle test or visit marking.
// Empty lists leave caller-provided outputs untouched. Hull queries are excluded.
inline void sourceStationaryDisplacementNormalCandidate(
 const std::vector<SourceDisplacementRuntimeMeshCandidate>&meshes,
 const std::vector<uint32_t>&ordered,const std::array<float,3>&point,
 uint32_t mask,std::array<float,3>&normal,uint32_t&contents){
 for(float v:point)if(!std::isfinite(v))throw std::runtime_error("nonfinite stationary normal point");
 if(ordered.empty())return;
 // Validate before committing outputs, including malformed later references.
 for(auto index:ordered){const auto&m=meshes.at(index);for(float v:m.baseNormal)if(!std::isfinite(v))throw std::runtime_error("nonfinite displacement base normal");const auto&b=m.tree.expandedBounds;for(size_t a=0;a<3;++a)if(!std::isfinite(b[a])||!std::isfinite(b[a+3])||b[a]>b[a+3])throw std::runtime_error("invalid stationary displacement bounds");}
 auto selected=meshes.at(ordered.front()).baseNormal;uint32_t selectedContents=1;
 for(auto index:ordered){const auto&m=meshes.at(index);if(!(m.contents&mask))continue;const auto&b=m.tree.expandedBounds;bool inside=true;for(size_t a=0;a<3;++a)if(point[a]<b[a]||point[a]>b[a+3])inside=false;if(inside){selected=m.baseNormal;selectedContents=m.contents;break;}}
 normal=selected;contents=selectedContents;
}
