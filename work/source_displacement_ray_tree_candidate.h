#pragma once
#include "source_displacement_collision_tree_candidate.h"
#include "source_displacement_collision_triangle_candidate.h"
#include "source_displacement_ray_triangle_candidate.h"
#include <emmintrin.h>

struct SourceDisplacementRayTreeHitCandidate {
 float fraction=1.f;
 uint32_t triangle=UINT32_MAX;
};

// Original zero-extent traversal 1015A130: breadth-first child bounds, then
// all selected leaf pairs in order. Current hit fraction does not prune boxes.
// Caller supplies start+delta and its finite inverse (FLT_MAX for zero delta).
inline SourceDisplacementRayTreeHitCandidate sourceDisplacementRayTreeCandidate(
 const std::vector<std::array<float,3>>&vertices,
 const std::vector<SourceDisplacementCollisionTriangleCandidate>&triangles,
 const SourceDisplacementCollisionTreeCandidate&tree,const std::array<float,3>&start,
 const std::array<float,3>&delta,const std::array<float,3>&inverse,bool oneSided,
 SourceDisplacementRayTreeHitCandidate initial={}){
 const size_t leaves=tree.leafPairs.size();if(leaves!=4&&leaves!=16&&leaves!=64&&leaves!=256)throw std::runtime_error("unsupported displacement tree shape");
 const size_t internal=(leaves-1)/3;if(tree.nodes.size()!=internal||triangles.size()!=2*leaves)throw std::runtime_error("displacement ray tree coverage");
 for(float value:start)if(!std::isfinite(value))throw std::runtime_error("nonfinite tree ray start");for(float value:delta)if(!std::isfinite(value))throw std::runtime_error("nonfinite tree ray delta");for(float value:inverse)if(!std::isfinite(value))throw std::runtime_error("nonfinite tree ray inverse");
 if(!std::isfinite(initial.fraction)||initial.fraction<0.f||initial.fraction>1.f)throw std::runtime_error("invalid prior displacement fraction");
 if(initial.triangle!=UINT32_MAX&&initial.triangle>=triangles.size())throw std::runtime_error("invalid prior triangle");
 for(const auto&p:vertices)for(float value:p)if(!std::isfinite(value))throw std::runtime_error("nonfinite tree ray vertex");
 for(const auto&t:triangles)for(auto index:t.packedVertices)if(size_t(index&511u)>=vertices.size())throw std::runtime_error("tree ray vertex reference");
 for(const auto&p:tree.leafPairs)for(auto index:p)if(index>=triangles.size())throw std::runtime_error("tree ray triangle reference");
 for(const auto&node:tree.nodes)for(size_t a=0;a<3;++a)for(size_t c=0;c<4;++c)if(!std::isfinite(node.bounds[a*4+c])||!std::isfinite(node.bounds[12+a*4+c])||node.bounds[a*4+c]>node.bounds[12+a*4+c])throw std::runtime_error("invalid tree ray child bounds");
 auto minimum=[](float a,float b){return _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 auto maximum=[](float a,float b){return _mm_cvtss_f32(_mm_max_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 std::vector<uint32_t>queue{0};queue.reserve(internal+leaves);size_t head=0;
 while(head<queue.size()&&queue[head]<internal){
  const uint32_t nodeId=queue[head++];const auto&node=tree.nodes[nodeId];
  for(uint32_t c=0;c<4;++c){
   std::array<float,3>near{},far{};
   for(size_t a=0;a<3;++a){const float lowRelative=node.bounds[a*4+c]-start[a],highRelative=node.bounds[12+a*4+c]-start[a];const float low=lowRelative-.03125f,high=highRelative+.03125f;const float t0=low*inverse[a],t1=high*inverse[a];near[a]=minimum(t0,t1);far[a]=maximum(t0,t1);}
   const float nearXY=maximum(near[0],near[1]),farXY=minimum(far[0],far[1]);
   const float entry=maximum(maximum(nearXY,near[2]),0.f),exit=minimum(minimum(farXY,far[2]),1.f);
   if(entry<=exit)queue.push_back(4*nodeId+1+c);
  }
 }
 auto result=initial;
 for(;head<queue.size();++head){
  const size_t leaf=size_t(queue[head])-internal;if(leaf>=leaves)throw std::runtime_error("tree leaf outside grid");
  for(auto id:tree.leafPairs[leaf]){
   const auto&t=triangles[id];
   const std::array<std::array<float,3>,3>points={vertices[t.packedVertices[0]&511u],vertices[t.packedVertices[2]&511u],vertices[t.packedVertices[1]&511u]};
   const float hit=sourceDisplacementRayTriangleCandidate(start,delta,points,oneSided);
   if(hit>=0.f&&hit<result.fraction){result.fraction=hit;result.triangle=id;}
  }
 }
 return result;
}

struct SourceDisplacementRayTraceCandidate {
 float fraction=1.f;
 std::array<float,3>normal{99,98,97};
 float distance=96.f;
 uint16_t displacementFlags=0xbeef;
};
// Original 1015B520 wrapper: ineligible/no-closer-hit leaves the prior trace
// untouched. The caller's world contents/surface updates are not performed.
inline bool sourceDisplacementRayTraceCandidate(
 const std::vector<std::array<float,3>>&vertices,
 const std::vector<SourceDisplacementCollisionTriangleCandidate>&triangles,
 const SourceDisplacementCollisionTreeCandidate&tree,uint32_t runtimeFlags,uint32_t contents,
 const std::array<float,3>&start,const std::array<float,3>&delta,
 const std::array<float,3>&inverse,bool oneSided,SourceDisplacementRayTraceCandidate&trace){
 if((runtimeFlags&8u)||!(contents&0x4081u))return false;
 const auto hit=sourceDisplacementRayTreeCandidate(vertices,triangles,tree,start,delta,inverse,oneSided,{trace.fraction,UINT32_MAX});
 if(hit.triangle==UINT32_MAX)return false;
 const auto&triangle=triangles.at(hit.triangle);auto staged=trace;staged.fraction=hit.fraction;staged.normal=triangle.normal;staged.distance=triangle.distance;staged.displacementFlags=uint16_t((triangle.flags>>6)&31u);trace=staged;return true;
}
