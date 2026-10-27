#pragma once
#include "material_session_store_bindings_candidate.h"
#include "material_find_file_request_candidate.h"

// Opt-in filename resource + CPU parameter service for an actual owned store.
// Caller supplies read-only logical GAME VMT reads and an explicit proxy factory.
// No native FindMaterial miss/error-resource equivalence, texture/GPU or renderer
// admission is implied. Store outlives this owner-thread service and callbacks.
class MaterialSessionFileRuntimeCandidate {
 MaterialObjectStoreCandidate&store;
 MaterialObjectParameterOptionsCandidate options;
 bool busy=false;
 struct Guard{bool&flag;explicit Guard(bool&b)noexcept:flag(b){flag=true;}~Guard(){flag=false;}Guard(const Guard&)=delete;};
public:
 explicit MaterialSessionFileRuntimeCandidate(MaterialObjectStoreCandidate&s,MaterialObjectParameterOptionsCandidate input):store(s),options(std::move(input)){}
 MaterialSessionFileRuntimeCandidate(const MaterialSessionFileRuntimeCandidate&)=delete;
 MaterialSessionFileRuntimeCandidate&operator=(const MaterialSessionFileRuntimeCandidate&)=delete;
 std::optional<MaterialObjectStoreCandidate::Id>findMaterial(std::string_view name){
  if(busy)return {};Guard guard(busy);
  try{
   std::string key;if(!materialFindCacheKeyCandidate(name,key))return {};
   const auto cached=store.find(key,false);if(cached)return cached; // NO Init or AddRef on a cached hit.
   MaterialFindFileRequestCandidate request;if(!options.provider||!materialFindFileRequestCandidate(name,request)||request.pathID!="GAME")return {};
   // Parser/patch validation precedes object publication. Unsupported or missing
   // data is unavailable, not a fabricated native null/error material.
   auto file=sourceMaterialPatchFilesCandidate(key,options.provider,false,true);
   if(file.status==SourceMaterialPatchFileLoadStatusCandidate::RootUnavailable||file.status==SourceMaterialPatchFileLoadStatusCandidate::RetainRootName||file.document.roots.size()!=1)return {};
   const auto reentrantCached=store.find(key,false);if(reentrantCached)return reentrantCached;
   MaterialObjectStoreCandidate::Id id=0;
   if(!store.createFileMaterial(key,"Other textures",id))return {};return id;
  }catch(...){return {};}
 }
 bool initialize(MaterialObjectStoreCandidate::Id id,const MaterialObjectStoreCandidate::Permit&permit={},MaterialObjectParameterAttemptCandidate*attempt=nullptr){
  if(attempt)*attempt={};if(busy)return false;Guard guard(busy);
  try{
   const auto*p=store.get(id);if(!p||p->source||options.externalSource||options.externalPatch||store.find(p->name,false)!=id)return false;
   return store.initializeParameters(id,options,permit,attempt);
  }catch(...){return false;}
 }
};
