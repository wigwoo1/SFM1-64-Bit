#pragma once
#include "engine_projection_normalize_candidate.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

struct SourceDisplacementCollisionTriangleCandidate {
 std::array<uint16_t,3>packedVertices{};
 uint16_t flags=0;
 std::array<float,3>normal{};
 float distance=0;
};
static_assert(sizeof(SourceDisplacementCollisionTriangleCandidate)==24,"collision triangle record size");

// Original plane helper 10159170 followed by extrema helper 101592E0.
// Preserves preexisting bits outside the helpers' masks. Tag/alpha setup in
// the enclosing loader is a separate step; no whole-loader claim here.
inline SourceDisplacementCollisionTriangleCandidate sourceDisplacementCollisionTriangleCandidate(
 const std::vector<std::array<float,3>>&vertices,const SourceDisplacementCollisionTriangleCandidate&initial){
 std::array<std::array<float,3>,3>points{};
 for(size_t i=0;i<3;++i){const size_t index=initial.packedVertices[i]&511u;if(index>=vertices.size())throw std::runtime_error("collision triangle vertex outside array");points[i]=vertices[index];for(float v:points[i])if(!std::isfinite(v))throw std::runtime_error("nonfinite collision triangle vertex");}
 auto result=initial;std::array<float,3>first{},second{};
 for(size_t a=0;a<3;++a){first[a]=points[1][a]-points[0][a];second[a]=points[2][a]-points[0][a];}
 // Native uses the reverse of the usual first-edge cross second-edge.
 for(size_t a=0;a<3;++a){const size_t b=(a+1)%3,c=(a+2)%3;const float left=second[b]*first[c],right=second[c]*first[b];result.normal[a]=left-right;}
 for(float v:result.normal)if(!std::isfinite(v))throw std::runtime_error("collision triangle cross product overflow");
 engineProjectionNormalizeCandidate(result.normal.data());
 for(float v:result.normal)if(!std::isfinite(v))throw std::runtime_error("nonfinite collision triangle normal");
 const float x=result.normal[0]*points[0][0],y=points[0][1]*result.normal[1],z=points[0][2]*result.normal[2];const float xy=y+x;result.distance=xy+z;
 if(!std::isfinite(result.distance))throw std::runtime_error("collision triangle plane distance overflow");
 result.flags=uint16_t((result.flags&0xffe8u)|0x28u);
 for(unsigned a=0;a<3;++a){if(result.normal[a]<0.f)result.flags=uint16_t(result.flags|(1u<<a));if(result.normal[a]==1.f)result.flags=uint16_t((result.flags&~0x38u)|(a<<3));}
 for(size_t a=0;a<3;++a){
  uint16_t low=0,high=0;float minimum=points[0][a],maximum=points[0][a];
  if(minimum>points[1][a]){minimum=points[1][a];low=1;}if(minimum>points[2][a])low=2;
  if(points[1][a]>maximum){maximum=points[1][a];high=1;}if(points[2][a]>maximum)high=2;
  result.packedVertices[a]=uint16_t((result.packedVertices[a]&0xe1ffu)|((low|(high<<2))<<9));
 }
 return result;
}
