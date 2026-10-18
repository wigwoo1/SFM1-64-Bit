#pragma once
#include "model_render_inputs_candidate.h"
#include <algorithm>
#include <cmath>
// Descending mean camera depth, weighted by indexed triangle corners.
// Range sorting cannot resolve intersecting surfaces or self-overlap.
template<class Inputs,class Vertices,class Depth>
std::vector<size_t> translucentRangeOrderCandidate(const Inputs&inputs,const Vertices&vertices,Depth depth){
 std::vector<std::pair<size_t,double>>keys;
 for(size_t n=0;n<inputs.ranges.size();++n){const auto&r=inputs.ranges[n];if(r.materialIndex>=inputs.materials.size())throw std::runtime_error("sort material index");if(!inputs.materials[r.materialIndex].translucent)continue;if(r.firstIndex>inputs.indices.size()||r.indexCount>inputs.indices.size()-r.firstIndex||r.indexCount%3)throw std::runtime_error("sort index span");double sum=0;for(size_t i=0;i<r.indexCount;++i){auto index=inputs.indices[r.firstIndex+i];if(index>=vertices.size())throw std::runtime_error("sort vertex index");double value=depth(vertices[index]);if(!std::isfinite(value))throw std::runtime_error("sort nonfinite depth");sum+=value;}double mean=r.indexCount?sum/r.indexCount:0;if(!std::isfinite(mean))throw std::runtime_error("sort depth overflow");keys.emplace_back(n,mean);}
 std::stable_sort(keys.begin(),keys.end(),[](auto&a,auto&b){return a.second>b.second;});std::vector<size_t>result;for(auto&key:keys)result.push_back(key.first);return result;
}
