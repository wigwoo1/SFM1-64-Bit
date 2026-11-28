#pragma once
#include "source_displacement_runtime_mesh_candidate.h"
#include "source_triangle_box_overlap_candidate.h"

// Prepared tree branch of stationary mesh box-overlap1015a7d0. Child bounds
// are inclusive, queued in ascending order. Only after the internal-node phase
// completes are selected leaf triangle pairs tested, first triangle then second.
template<class TriangleOverlap>
bool sourceDisplacementBoxWalkCandidate(const SourceDisplacementCollisionTreeCandidate& tree,
 uint32_t runtimeFlags,const std::array<float,3>& minimum,const std::array<float,3>& maximum,
 TriangleOverlap overlap){
 if(runtimeFlags&4u)return false;
 if(tree.nodes.empty()||tree.nodes.size()>85||tree.leafPairs.size()!=3*tree.nodes.size()+1)throw std::runtime_error("unsupported displacement box tree coverage");
 for(size_t a=0;a<3;++a)if(!std::isfinite(minimum[a])||!std::isfinite(maximum[a])||minimum[a]>maximum[a])throw std::runtime_error("invalid displacement box bounds");
 std::vector<uint32_t> queue{0};queue.reserve(tree.nodes.size()+tree.leafPairs.size());size_t head=0;
 while(head<queue.size()&&queue[head]<tree.nodes.size()){
  const uint32_t index=queue[head++];const auto& node=tree.nodes.at(index);
  for(uint32_t child=0;child<4;++child){
   bool inside=true;
   for(size_t a=0;a<3;++a){const float low=node.bounds[4*a+child],high=node.bounds[12+4*a+child];if(!std::isfinite(low)||!std::isfinite(high)||low>high)throw std::runtime_error("invalid displacement box child bounds");if(std::max(minimum[a],low)>std::min(maximum[a],high))inside=false;}
   if(inside){if(queue.size()>=341)throw std::runtime_error("displacement box queue limit");queue.push_back(4*index+1+child);}
  }
 }
 for(;head<queue.size();++head){const auto id=queue[head];if(id<tree.nodes.size())throw std::runtime_error("unbalanced displacement box tree");const auto& pair=tree.leafPairs.at(id-tree.nodes.size());if(overlap(pair[0])||overlap(pair[1]))return true;}
 return false;
}

inline bool sourceDisplacementBoxOverlapCandidate(const SourceDisplacementRuntimeMeshCandidate& mesh,
 const std::array<float,3>& minimum,const std::array<float,3>& maximum){
 if(mesh.runtimeFlags&4u)return false;
 std::array<float,3> center{},extents{};for(size_t a=0;a<3;++a){center[a]=(maximum[a]+minimum[a])*.5f;extents[a]=maximum[a]-center[a];if(!std::isfinite(center[a])||!std::isfinite(extents[a]))throw std::runtime_error("displacement box midpoint overflow");}
 return sourceDisplacementBoxWalkCandidate(mesh.tree,mesh.runtimeFlags,minimum,maximum,[&](uint32_t id){
  const auto& triangle=mesh.triangles.at(id);std::array<std::array<float,3>,3> points{};
  for(size_t p=0;p<3;++p)points[p]=mesh.vertices.at(triangle.packedVertices[p==0?0:3-p]&511u);
  return sourceTriangleBoxOverlapCandidate(center,extents,points,triangle.normal,triangle.distance,uint8_t((triangle.flags>>3)&7u),uint8_t(triangle.flags&7u));
 });
}
