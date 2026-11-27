#pragma once
#include "source_displacement_collision_tree_candidate.h"
#include "engine_projection_normalize_candidate.h"

// Non-ray displacement hull traversal1015c840, not triangle contact arithmetic.
// Host owns beginTriangles/traceTriangle/endTriangles and updates staged fraction.
template<class Host>
bool sourceDisplacementHullTreeCandidate(const SourceDisplacementCollisionTreeCandidate& tree,
 uint32_t runtimeFlags,const std::array<float,3>& start,const std::array<float,3>& delta,
 const std::array<float,3>& inverse,const std::array<float,3>& extents,float& fraction,Host& host) {
 if(runtimeFlags&4u)return false;
 const size_t leaves=tree.leafPairs.size(),internal=(leaves-1)/3;
 if((leaves!=4&&leaves!=16&&leaves!=64&&leaves!=256)||tree.nodes.size()!=internal)throw std::runtime_error("invalid hull displacement tree");
 if(!std::isfinite(fraction)||fraction<0||fraction>1)throw std::runtime_error("invalid hull displacement fraction");
 std::array<float,3> expanded{},normalized=delta;
 for(size_t a=0;a<3;++a){if(!std::isfinite(start[a])||!std::isfinite(delta[a])||!std::isfinite(inverse[a])||!std::isfinite(extents[a])||extents[a]<0)throw std::runtime_error("invalid displacement hull");expanded[a]=extents[a]+.03125f;if(!std::isfinite(expanded[a]))throw std::runtime_error("displacement hull expansion overflow");}
 for(const auto& pair:tree.leafPairs)for(auto id:pair)if(id>=2*leaves)throw std::runtime_error("invalid hull displacement triangle reference");
 engineProjectionNormalizeCandidate(normalized.data());
 for(const auto& node:tree.nodes)for(size_t a=0;a<3;++a)for(size_t c=0;c<4;++c)if(!std::isfinite(node.bounds[a*4+c])||!std::isfinite(node.bounds[12+a*4+c])||node.bounds[a*4+c]>node.bounds[12+a*4+c])throw std::runtime_error("invalid hull displacement bounds");
 auto minimum=[](float a,float b){return _mm_cvtss_f32(_mm_min_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 auto maximum=[](float a,float b){return _mm_cvtss_f32(_mm_max_ss(_mm_set_ss(a),_mm_set_ss(b)));};
 std::vector<uint32_t> queue{0};queue.reserve(internal+leaves);size_t head=0;
 while(head<queue.size()&&queue[head]<internal){const auto nodeId=queue[head++];const auto& node=tree.nodes[nodeId];
  for(uint32_t child=0;child<4;++child){std::array<float,3> entryTimes{},exitTimes{};for(size_t a=0;a<3;++a){const float lowRelative=node.bounds[a*4+child]-start[a],highRelative=node.bounds[12+a*4+child]-start[a],low=lowRelative-expanded[a],high=highRelative+expanded[a],t0=low*inverse[a],t1=high*inverse[a];entryTimes[a]=minimum(t0,t1);exitTimes[a]=maximum(t0,t1);}const float nearXY=maximum(entryTimes[0],entryTimes[1]),farXY=minimum(exitTimes[0],exitTimes[1]);const float entry=maximum(maximum(nearXY,entryTimes[2]),0.f),exit=minimum(minimum(farXY,exitTimes[2]),1.f);if(entry<=exit)queue.push_back(4*nodeId+1+child);}
 }
 if(head==queue.size())return false;
 const float previous=fraction;float staged=fraction;host.beginTriangles();
 try{for(;head<queue.size();++head){const auto& pair=tree.leafPairs.at(size_t(queue[head])-internal);for(auto id:pair)host.traceTriangle(id,normalized,staged);}}
 catch(...){host.endTriangles();throw;}
 host.endTriangles();if(!std::isfinite(staged)||staged<0||staged>1)throw std::runtime_error("invalid hull triangle callback fraction");fraction=staged;return previous>fraction;
}
