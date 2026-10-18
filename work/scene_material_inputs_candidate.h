#pragma once
#include "scene_render_inputs_cache_candidate.h"
#include "session_material_binding_candidate.h"
#include "session_material_alpha_candidate.h"
#include <memory>
struct SceneMaterialInputsCandidate {
 std::vector<std::shared_ptr<const ModelRenderInputsCandidate>>owners;
 std::vector<const ModelRenderInputsCandidate*>entries;
 std::vector<std::vector<SessionMaterialAlphaCandidate>>alpha;
 size_t size()const{return entries.size();}
 const ModelRenderInputsCandidate&at(size_t model)const{return *entries.at(model);}
 const ModelRenderInputsCandidate&operator[](size_t model)const{return at(model);}
 std::shared_ptr<const ModelRenderInputsCandidate>ownerAt(size_t model)const{return model<owners.size()?owners[model]:nullptr;}
 size_t appendOwned(std::shared_ptr<const ModelRenderInputsCandidate>input){if(!input)throw std::runtime_error("owned scene material missing");if(owners.size()>entries.size()||alpha.size()!=entries.size())throw std::runtime_error("scene material owner coverage");owners.resize(entries.size());entries.reserve(entries.size()+1);alpha.reserve(alpha.size()+1);owners.reserve(owners.size()+1);size_t index=entries.size();entries.push_back(input.get());alpha.emplace_back();owners.push_back(std::move(input));return index;}
};
// Cache owns decoded inputs; references stay valid for the cache lifetime.
// Build inputs before GPU submission; supported missing textures carry explicit
// diagnostics and a visible fallback rather than discarding the full scene.
template<class Models>
SceneMaterialInputsCandidate readSceneMaterialInputsCandidate(const Graph&graph,const Models&models,SceneRenderInputsCacheCandidate&cache,const std::vector<std::filesystem::path>&mounts){
 SceneMaterialInputsCandidate result;result.entries.reserve(models.size());result.alpha.reserve(models.size());
 for(const auto&model:models){auto partition=partitionSessionMaterialAlphaCandidate(readSessionMaterialOverridesCandidate(graph,model.id));auto key=sessionMaterialOverrideKeyCandidate(partition.resources);auto binding=sessionMaterialBindingCandidate(partition.resources,mounts);const auto&input=cache.get(model.placement.asset,model.skin,model.body,key,binding);result.entries.push_back(&input);result.alpha.push_back(std::move(partition.alpha));}return result;
}
