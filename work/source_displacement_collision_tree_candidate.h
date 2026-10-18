#pragma once
#include "source_displacement_pair_bounds_candidate.h"
#include <functional>

struct alignas(16) SourceDisplacementCollisionNodeCandidate {
 // Native 96-byte layout: min[x/y/z][four children], max[x/y/z][four].
 std::array<float,24>bounds{};
};
struct SourceDisplacementCollisionTreeCandidate {
 std::vector<std::array<uint16_t,2>>leafPairs;
 std::vector<SourceDisplacementCollisionNodeCandidate>nodes;
 std::array<float,6>tightBounds{},expandedBounds{};
};

// Native builder 1015B8E0 and bounds recursion 10159A40. This constructs
// tree bounds only; it does not create triangle planes/edge collision metadata.
inline SourceDisplacementCollisionTreeCandidate sourceDisplacementCollisionTreeCandidate(
 uint32_t power,const std::vector<std::array<float,3>>&vertices,
 const std::vector<std::array<uint16_t,3>>&packedTriangles){
 if(power<1||power>4)throw std::runtime_error("collision tree power outside supported range");
 const uint32_t cells=1u<<power,leafCount=cells*cells,internalCount=(leafCount-1)/3;
 if(vertices.size()!=size_t(cells+1)*(cells+1)||packedTriangles.size()!=size_t(leafCount)*2)throw std::runtime_error("collision tree grid coverage mismatch");
 for(const auto&point:vertices)for(float value:point)if(!std::isfinite(value))throw std::runtime_error("nonfinite collision tree vertex");
 for(const auto&triangle:packedTriangles)for(auto index:triangle)if(size_t(index&511u)>=vertices.size())throw std::runtime_error("collision tree vertex reference outside grid");
 SourceDisplacementCollisionTreeCandidate result;
 result.leafPairs.resize(leafCount);result.nodes.resize(internalCount);
 for(uint32_t row=0;row<cells;++row)for(uint32_t column=0;column<cells;++column){
  uint32_t morton=0;for(uint32_t bit=0;bit<power;++bit){morton|=((column>>bit)&1)<<(2*bit);morton|=((row>>bit)&1)<<(2*bit+1);}
  const uint32_t firstTriangle=2*(row*cells+column);
  result.leafPairs[morton]={uint16_t(firstTriangle),uint16_t(firstTriangle+1)};
 }
 std::function<std::array<float,6>(uint32_t)>build=[&](uint32_t node){
  if(node>=internalCount){const auto&pair=result.leafPairs.at(node-internalCount);std::array<uint16_t,6>packed{};for(size_t t=0;t<2;++t)for(size_t a=0;a<3;++a)packed[3*t+a]=packedTriangles.at(pair[t])[a];return sourceDisplacementPairBoundsCandidate(vertices,packed);}
  std::array<float,6>bounds;for(size_t a=0;a<3;++a){bounds[a]=std::numeric_limits<float>::max();bounds[3+a]=-std::numeric_limits<float>::max();}
  for(uint32_t child=0;child<4;++child){
   const auto b=build(4*node+1+child);
   for(size_t a=0;a<3;++a){
    result.nodes[node].bounds[a*4+child]=b[a];result.nodes[node].bounds[12+a*4+child]=b[3+a];
    // Original merges each child's min point, then its max point.
    if(b[a]<bounds[a])bounds[a]=b[a];if(b[a]>bounds[3+a])bounds[3+a]=b[a];
    if(b[3+a]<bounds[a])bounds[a]=b[3+a];if(b[3+a]>bounds[3+a])bounds[3+a]=b[3+a];
   }
  }
  return bounds;
 };
 result.tightBounds=build(0);
 for(size_t a=0;a<3;++a){result.expandedBounds[a]=result.tightBounds[a]-1.f;result.expandedBounds[3+a]=result.tightBounds[3+a]+1.f;}
 return result;
}
