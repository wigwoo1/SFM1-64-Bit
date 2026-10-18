#pragma once
#include "source_displacement_collision_tree_candidate.h"
#include "source_displacement_collision_triangle_candidate.h"
#include "source_displacement_grid_triangles_candidate.h"

struct SourceDisplacementRuntimeMeshCandidate {
 uint32_t power=0,contents=0,runtimeFlags=0;
 std::array<std::array<float,3>,4>corners{};
 std::array<float,3>baseNormal{};
 std::vector<std::array<float,3>>vertices;
 std::vector<SourceDisplacementCollisionTriangleCandidate>triangles;
 SourceDisplacementCollisionTreeCandidate tree;
};

// Reconstructs semantic mesh fields/arrays of 1015C6B0 -> 1015BF90 from
// already prepared CoreDisp inputs. Original corner/position/alpha loading
// and material-surface binding are separate, not silently inferred here.
inline SourceDisplacementRuntimeMeshCandidate sourceDisplacementRuntimeMeshCandidate(
 uint32_t power,uint32_t contents,uint32_t runtimeFlags,
 const std::array<std::array<float,3>,4>&corners,
 const std::vector<std::array<float,3>>&positions,const std::vector<float>&alphas,
 const std::vector<SourceDisplacementGridTriangleCandidate>&sourceTriangles){
 if(power<1||power>4)throw std::runtime_error("runtime mesh power outside supported range");
 const size_t cells=size_t(1)<<power,vertexCount=(cells+1)*(cells+1),triangleCount=cells*cells*2;
 if(positions.size()!=vertexCount||alphas.size()!=vertexCount||sourceTriangles.size()!=triangleCount)throw std::runtime_error("runtime mesh grid coverage mismatch");
 for(const auto&v:positions)for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite runtime position");
 for(const auto&v:corners)for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite runtime corner");
 for(float x:alphas)if(!std::isfinite(x))throw std::runtime_error("nonfinite runtime alpha");
 for(const auto&t:sourceTriangles)for(auto i:t.vertices)if(size_t(i&511u)>=vertexCount)throw std::runtime_error("runtime triangle vertex outside grid");
 SourceDisplacementRuntimeMeshCandidate result;result.power=power;result.contents=contents;result.runtimeFlags=runtimeFlags;result.corners=corners;result.vertices=positions;
 std::array<float,3>edge3{},edge1{};for(size_t a=0;a<3;++a){edge3[a]=corners[3][a]-corners[0][a];edge1[a]=corners[1][a]-corners[0][a];}
 for(size_t a=0;a<3;++a){const size_t b=(a+1)%3,c=(a+2)%3;const float left=edge3[b]*edge1[c],right=edge3[c]*edge1[b];result.baseNormal[a]=left-right;}
 engineProjectionNormalizeCandidate(result.baseNormal.data());for(float x:result.baseNormal)if(!std::isfinite(x))throw std::runtime_error("nonfinite runtime base normal");
 std::vector<std::array<uint16_t,3>>packed;packed.reserve(triangleCount);result.triangles.reserve(triangleCount);
 for(const auto&source:sourceTriangles){
  SourceDisplacementCollisionTriangleCandidate triangle;
  for(size_t i=0;i<3;++i)triangle.packedVertices[i]=uint16_t(source.vertices[i]&511u);
  triangle.flags=uint16_t((uint32_t(source.tags)<<6)&0x7c0u);
  const float firstTwo=alphas[triangle.packedVertices[0]]+alphas[triangle.packedVertices[1]];
  const float sum=firstTwo+alphas[triangle.packedVertices[2]];
  if(!std::isfinite(sum))throw std::runtime_error("runtime alpha sum overflow");
  triangle.flags=uint16_t(triangle.flags|(sum>382.5f?0x400u:0x200u)|0x40u);
  triangle=sourceDisplacementCollisionTriangleCandidate(positions,triangle);packed.push_back(triangle.packedVertices);result.triangles.push_back(triangle);
 }
 result.tree=sourceDisplacementCollisionTreeCandidate(power,result.vertices,packed);return result;
}
