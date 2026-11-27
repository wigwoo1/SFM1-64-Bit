#pragma once
#include "source_displacement_collision_triangle_candidate.h"

// Triangle cache geometry1015c3b0 ->1015bae0/bbf0/bcf0. Host.intern owns
// lookup/dedup and signed-index encoding; these semantics are not assumed here.
template<class Host>
std::array<uint16_t,9> sourceDisplacementEdgePlanesCandidate(
 const std::vector<std::array<float,3>>& vertices,
 const SourceDisplacementCollisionTriangleCandidate& triangle,Host& host) {
 std::array<std::array<float,3>,3> points{};for(size_t i=0;i<3;++i){points[i]=vertices.at(triangle.packedVertices[i]&511u);for(float v:points[i])if(!std::isfinite(v))throw std::runtime_error("invalid edge cache vertex");}
 std::array<uint16_t,9> result{};
 for(size_t edge=0;edge<3;++edge){const auto& start=points[edge];const auto& end=points[(edge+1)%3];const auto& other=points[(edge+2)%3];std::array<float,3> delta{};for(size_t a=0;a<3;++a)delta[a]=end[a]-start[a];
  for(size_t family=0;family<3;++family){std::array<float,3> plane{};
   if(family==0)plane={0,delta[2],-delta[1]};else if(family==1)plane={-delta[2],0,delta[0]};else plane={delta[1],-delta[0],0};
   engineProjectionNormalizeCandidate(plane.data());const size_t a=(family+1)%3,b=(family+2)%3;
   if(plane[a]==0||plane[b]==0){result[family*3+edge]=65535;continue;}
   const double first=double(start[a])*plane[a]+double(start[b])*plane[b];
   const double third=double(other[a])*plane[a]+double(other[b])*plane[b];float distance=float(first);
   if(std::abs(third-first)>=.03125&&float(third)>distance){for(auto& v:plane)v=-v;distance=-distance;}
   plane[family]=distance;for(float v:plane)if(!std::isfinite(v))throw std::runtime_error("edge cache plane overflow");
   result[family*3+edge]=host.intern(plane);
  }
 }
 return result;
}
