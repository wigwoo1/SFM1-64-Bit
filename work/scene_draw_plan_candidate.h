#pragma once
#include "scene_translucent_order_candidate.h"
struct SceneDrawPlanCandidate {
 std::vector<SceneDrawRangeCandidate> opaque;
 std::vector<SceneDrawRangeCandidate> translucent;
};
// Preserve model/range order for opaque draws; sort transparent draws globally.
template<class Models,class Inputs,class Depth>
SceneDrawPlanCandidate sceneDrawPlanCandidate(const Models&models,const Inputs&inputs,Depth depth){
 if(models.size()!=inputs.size())throw std::runtime_error("scene draw model count");
 SceneDrawPlanCandidate plan;
 for(size_t model=0;model<models.size();++model){
  if(!models[model].model.visible)continue;
  const auto&input=inputs[model];
  for(size_t range=0;range<input.ranges.size();++range){
   const auto&r=input.ranges[range];
   if(r.materialIndex>=input.materials.size())throw std::runtime_error("scene draw material index");
   if(r.firstIndex>input.indices.size()||r.indexCount>input.indices.size()-r.firstIndex||r.indexCount%3)throw std::runtime_error("scene draw index span");
   for(size_t n=0;n<r.indexCount;++n)if(input.indices[r.firstIndex+n]>=models[model].vertices.size())throw std::runtime_error("scene draw vertex index");
   if(!input.materials[r.materialIndex].translucent&&r.indexCount)plan.opaque.push_back({model,range,0});
  }
 }
 plan.translucent=sceneTranslucentOrderCandidate(models,inputs,depth);
 plan.translucent.erase(std::remove_if(plan.translucent.begin(),plan.translucent.end(),[&](const auto&draw){return inputs[draw.model].ranges[draw.range].indexCount==0;}),plan.translucent.end());
 return plan;
}
