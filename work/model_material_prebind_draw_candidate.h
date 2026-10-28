#pragma once
#include "sfm_model_geometry.h"
#include "mounted_model_material_candidate.h"
#include "session_model_material_proxy_context_candidate.h"
#include "studio_proxy_prebind_selection_candidate.h"
#include "material_object_store_candidate.h"
#include "material_find_cache_key_candidate.h"

// Borrowed caller-retained filename material. Private store identity prevents a
// numerically colliding ID from another store being accepted by this adapter.
// Does not acquire/own a reference; store and caller's reference outlive use.
class ModelMaterialResourceBindingCandidate {
 const MaterialObjectStoreCandidate*owner=nullptr;
 MaterialObjectStoreCandidate::Id id=0;
 std::string name;
public:
 ModelMaterialResourceBindingCandidate()=default;
 static std::optional<ModelMaterialResourceBindingCandidate>checked(MaterialObjectStoreCandidate&store,MaterialObjectStoreCandidate::Id id)noexcept{
  try{const auto*p=store.get(id);if(!p||p->references.load()<=0||!p->parameterState||!(p->flags&4)||p->initializationPending||p->releasePending||p->removalPending||p->evaluationPending||p->sessionPending||store.find(p->name,false)!=id)return {};
   ModelMaterialResourceBindingCandidate out;out.owner=&store;out.id=id;out.name=p->name;return out;
  }catch(...){return {};}
 }
 const MaterialObjectStoreCandidate::Object*read(MaterialObjectStoreCandidate&store)const{
  if(owner!=&store)return nullptr;const auto*p=store.get(id);
  if(!p||p->name!=name||p->references.load()<=0||!p->parameterState||!(p->flags&4)||p->initializationPending||p->releasePending||p->removalPending||p->evaluationPending||p->sessionPending||store.find(name,false)!=id||(p->proxyState&&!p->proxyState->complete()))return nullptr;
  return p;
 }
};
struct ModelMaterialPrebindDrawViewCandidate {
 const StudioProxyPrebindPlanCandidate&selection;
 const MaterialObjectStoreCandidate::SessionDrawView&draw;
};

// Ordinary IFM-puppet, already decoded MDL/VTX LOD0, caller-retained materials.
// Asset loader association (assetKey -> immutable geometry) is caller-owned;
// this checks that key against the current session identity, not file provenance.
// No asset reads, loader fallback, clock inference, GPU or editor admission.
inline bool withModelMaterialPrebindDrawCandidate(MaterialObjectStoreCandidate&store,
 const Graph&graph,const SessionRenderModelCandidate&model,const std::string&assetKey,
 const SfmModelGeometryCandidate&geometry,const std::vector<ModelMaterialResourceBindingCandidate>&bindings,
 const std::vector<MaterialObjectStoreCandidate::SessionEntry>&overrides,const MaterialProxyEvaluationInputsCandidate&inputs,
 const std::function<bool(const ModelMaterialPrebindDrawViewCandidate&)>&observer,int phase=0,const MaterialObjectStoreCandidate::Permit&permit={},const MaterialTextureResolverCandidate&textures={})noexcept {
 try{
  if(!observer)return false;const auto instance=model;const auto asset=assetKey;const auto use=observer;const auto textureLookup=textures;
  if(asset!=instance.placement.asset||instance.skin>INT32_MAX||instance.body>INT32_MAX)return false;
  auto context=sessionModelMaterialProxyContextCandidate(graph,instance);if(!context||!context->verifiedPuppetWithoutEntity())return false;
  auto validModel=[&](){auto current=sessionModelMaterialProxyContextCandidate(graph,instance);return current&&current->source().matches(instance.id,context->source().guid(),asset)&&sessionModelSkinCandidate(graph,instance.id)==instance.skin&&sessionModelBodyCandidate(graph,instance.id)==instance.body;};
  if(!validModel())return false;
  if(inputs.modelContext&&!inputs.modelContext->source().matches(instance.id,context->source().guid(),asset))return false;
  if(geometry.materials.names.size()>32768||bindings.size()!=geometry.materials.names.size()||geometry.bodyModelCounts.size()!=geometry.bodyBases.size()||geometry.bodyModelCounts.size()>4096)return false;
  StudioProxyPrebindInputCandidate input;input.textureCount=uint32_t(geometry.materials.names.size());input.drawSkin=int32_t(instance.skin);input.body=int32_t(instance.body);input.lod=0;
  for(const auto&family:geometry.materials.skins){std::vector<int16_t>row;row.reserve(family.size());for(auto index:family){if(index>INT16_MAX)return false;row.push_back(int16_t(index));}input.skinFamilies.push_back(std::move(row));}
  input.lodMaterials.resize(1);input.lodMaterials[0].resize(bindings.size());
  for(size_t i=0;i<bindings.size();++i){const auto*p=bindings[i].read(store);if(!p)continue;bool match=false;
   for(const auto&directory:geometry.materials.directories){auto relative=materialRelativePathCandidate(directory+geometry.materials.names[i],".vmt");std::string key;if(materialFindCacheKeyCandidate(relative.generic_string(),key)&&key==p->name)match=true;}
   if(match)input.lodMaterials[0][i]={p->id,bool(p->proxyState&&p->proxyState->size())};
  }
  size_t models=0;for(size_t i=0;i<geometry.bodyModelCounts.size();++i){auto count=geometry.bodyModelCounts[i],base=geometry.bodyBases[i];if(!count||count>4096||!base||base>INT32_MAX||count>65536-models)return false;models+=count;StudioProxyPrebindBodyCandidate body;body.base=int32_t(base);body.models.resize(count);input.bodies.push_back(std::move(body));}
  size_t meshes=0;for(const auto&mesh:geometry.meshes){if(mesh.body>=input.bodies.size()||mesh.model>=input.bodies[mesh.body].models.size()||mesh.mesh>=(1u<<20)||mesh.material<0||++meshes>(1u<<20))return false;auto&refs=input.bodies[mesh.body].models[mesh.model].meshSkinReferences;if(refs.size()<=mesh.mesh)refs.resize(size_t(mesh.mesh)+1,-1);if(refs[mesh.mesh]!=-1)return false;refs[mesh.mesh]=mesh.material;}
  for(const auto&body:input.bodies)for(const auto&part:body.models)for(auto reference:part.meshSkinReferences)if(reference<0)return false;
  StudioProxyPrebindPlanCandidate selection;if(!studioProxyPrebindSelectionCandidate(input,selection))return false;
  std::vector<MaterialObjectStoreCandidate::SessionDrawStep>steps;steps.reserve(selection.visits.size());auto frameInputs=inputs;frameInputs.modelContext=context;
  // A no-proxy material still needs a value snapshot and pending-scope guard.
  // Its empty list executes no proxy, so this is not an invented native OnBind.
  for(const auto&visit:selection.visits)steps.push_back({visit.material,frameInputs});
  return store.withSessionDrawSteps(overrides,steps,[&](const auto&draw){if(!validModel())return false;bool ok=use(ModelMaterialPrebindDrawViewCandidate{selection,draw});return ok&&validModel();},phase,permit,textureLookup);
 }catch(...){return false;}
}
