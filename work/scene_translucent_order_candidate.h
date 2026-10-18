#pragma once
#include "model_render_inputs_candidate.h"
#include <algorithm>
#include <cmath>
struct SceneDrawRangeCandidate {size_t model,range;double depth;};
// World-space range means, globally ordered across visible model instances.
// Intersections and per-triangle transparency remain unresolved.
template<class Models,class Inputs,class Depth>
std::vector<SceneDrawRangeCandidate>sceneTranslucentOrderCandidate(const Models&models,const Inputs&inputs,Depth depth){
 if(models.size()!=inputs.size())throw std::runtime_error("scene sort model count");std::vector<SceneDrawRangeCandidate>keys;
 for(size_t model=0;model<models.size();++model){if(!models[model].model.visible)continue;auto&source=inputs[model];auto&vertices=models[model].vertices;for(size_t range=0;range<source.ranges.size();++range){auto&r=source.ranges[range];if(r.materialIndex>=source.materials.size())throw std::runtime_error("scene sort material index");if(!source.materials[r.materialIndex].translucent)continue;if(r.firstIndex>source.indices.size()||r.indexCount>source.indices.size()-r.firstIndex||r.indexCount%3)throw std::runtime_error("scene sort index span");double sum=0;for(size_t n=0;n<r.indexCount;++n){auto index=source.indices[r.firstIndex+n];if(index>=vertices.size())throw std::runtime_error("scene sort vertex index");auto value=depth(vertices[index]);if(!std::isfinite(value))throw std::runtime_error("scene sort nonfinite depth");sum+=value;}double mean=r.indexCount?sum/r.indexCount:0;if(!std::isfinite(mean))throw std::runtime_error("scene sort depth overflow");keys.push_back({model,range,mean});}}
 std::stable_sort(keys.begin(),keys.end(),[](auto&a,auto&b){return a.depth>b.depth;});return keys;
}
