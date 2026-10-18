#pragma once
#include "scene_material_inputs_candidate.h"
#include <cmath>
struct SceneMaterialDrawViewCandidate {
 struct Material {bool translucent;};
 const std::vector<uint32_t>&indices;
 const std::vector<ModelRenderRangeCandidate>&ranges;
 std::vector<Material>materials;
 std::vector<float>alpha;
};
struct SceneMaterialDrawInputsCandidate {
 std::vector<SceneMaterialDrawViewCandidate>models;
 size_t size()const{return models.size();}
 const SceneMaterialDrawViewCandidate&operator[](size_t n)const{return models.at(n);}
};
inline SceneMaterialDrawInputsCandidate prepareSceneMaterialDrawInputsCandidate(const SceneMaterialInputsCandidate&inputs,const std::vector<std::filesystem::path>&mounts){
 if(inputs.alpha.size()!=inputs.size())throw std::runtime_error("scene alpha model count");
 SceneMaterialDrawInputsCandidate result;result.models.reserve(inputs.size());
 for(size_t model=0;model<inputs.size();++model){const auto&source=inputs[model];SceneMaterialDrawViewCandidate view{source.indices,source.ranges,{},{}};
  for(const auto&material:source.materials){float alpha=1;auto values=vmtValuesCandidate(material.source.syntax,"$alpha");if(values.size()>1)throw std::runtime_error("scene duplicate alpha");if(!values.empty()){size_t consumed=0;alpha=std::stof(values[0],&consumed);if(consumed!=values[0].size())throw std::runtime_error("scene alpha syntax");}
   std::string identity;for(const auto&mount:mounts){auto relative=material.source.vmt.lexically_normal().lexically_relative((mount/"materials").lexically_normal());if(relative.empty()||relative.is_absolute())continue;bool outside=false;for(const auto&part:relative)if(part=="..")outside=true;if(outside)continue;identity=normalizeSessionMaterialNameCandidate(relative.generic_string());break;}
   for(const auto&value:inputs.alpha[model])if(value.material==identity)alpha=value.alpha;
   if(!std::isfinite(alpha)||alpha<0||alpha>1)throw std::runtime_error("scene alpha range");view.alpha.push_back(alpha);view.materials.push_back({material.translucent||alpha<1});
  }result.models.push_back(std::move(view));
 }return result;
}
